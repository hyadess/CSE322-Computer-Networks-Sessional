#include "tcp-adaptive-reno.h"

#include "rtt-estimator.h"
#include "tcp-socket-base.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE("TcpAdaptiveReno");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TcpAdaptiveReno);

TypeId
TcpAdaptiveReno::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::TcpAdaptiveReno")
            .SetParent<TcpNewReno>()
            .SetGroupName("Internet")
            .AddConstructor<TcpAdaptiveReno>()
            .AddAttribute(
                "FilterType",
                "Use this to choose no filter or Tustin's approximation filter",
                EnumValue(TcpAdaptiveReno::TUSTIN),
                MakeEnumAccessor(&TcpAdaptiveReno::m_fType),
                MakeEnumChecker(TcpAdaptiveReno::NONE, "None", TcpAdaptiveReno::TUSTIN, "Tustin"))
            .AddTraceSource("EstimatedBW",
                            "The estimated bandwidth",
                            MakeTraceSourceAccessor(&TcpAdaptiveReno::m_currentBW),
                            "ns3::TracedValueCallback::Double");
    return tid;
}

TcpAdaptiveReno::TcpAdaptiveReno(void)
    : TcpWestwoodPlus(),
      m_rtt_min(Time(0)),
      m_rtt_current(Time(0)),
      m_jPacketLRtt(Time(0)),
      m_rtt_cong(Time(0)),
      m_rtt_prev_cong(Time(0)),
      m_W_inc(0),
      m_W_base(0),
      m_W_probe(0)
{
    NS_LOG_FUNCTION(this);
}

TcpAdaptiveReno::TcpAdaptiveReno(const TcpAdaptiveReno& sock)
    : TcpWestwoodPlus(sock),
      m_rtt_min(Time(0)),
      m_rtt_current(Time(0)),
      m_jPacketLRtt(Time(0)),
      m_rtt_cong(Time(0)),
      m_rtt_prev_cong(Time(0)),
      m_W_inc(0),
      m_W_base(0),
      m_W_probe(0)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Invoked the copy constructor");
}

TcpAdaptiveReno::~TcpAdaptiveReno(void)
{
}

/*
The function is called every time an ACK is received (only one time
also for cumulative ACKs) and contains timing information
*/
void
TcpAdaptiveReno::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << packetsAcked << rtt);

    if (rtt.IsZero())
    {
        NS_LOG_WARN("RTT measured is zero!");
        return;
    }

    m_ackedSegments += packetsAcked;

    // calculate min rtt here
    if (m_rtt_min.IsZero())
    {
        m_rtt_min = rtt;
    }
    else if (rtt <= m_rtt_min)
    {
        m_rtt_min = rtt;
    }

    m_rtt_current = rtt;
    

    TcpWestwoodPlus::EstimateBW(rtt, tcb);
}

double
TcpAdaptiveReno::EstimateCongestionLevel()
{
    float m_a = 0.85; 
    if (m_rtt_prev_cong < m_rtt_min)
        m_a = 0; // the initial value should take the full current Jth loss Rtt

    double conjRtt =
        m_a * m_rtt_prev_cong.GetSeconds() +
        (1 - m_a) * m_jPacketLRtt.GetSeconds(); // TODO : do i need to reset m_maxRtt value to this
    m_rtt_cong = Seconds(conjRtt);              // for next step calcu
    NS_LOG_LOGIC("conj rtt : " << m_rtt_cong << " ; m_rtt_prev_cong : " << m_rtt_prev_cong
                               << " ; j rtt : " << m_jPacketLRtt);

    return std::min((m_rtt_current.GetSeconds() - m_rtt_min.GetSeconds()) /
                        (conjRtt - m_rtt_min.GetSeconds()),
                    1.0);
}

void
TcpAdaptiveReno::EstimateIncWnd(Ptr<TcpSocketState> tcb)
{
    double congestionEstimation = EstimateCongestionLevel();

    
    double m_W_inc_max = static_cast<double>(m_currentBW.Get().GetBitRate());

    double alpha = 10; // 2 10
    double beta = 2 * m_W_inc_max * ((1 / alpha) - ((1 / alpha + 1) / (std::exp(alpha))));
    double gamma = 1 - (2 * m_W_inc_max * ((1 / alpha) - ((1 / alpha + 0.5) / (std::exp(alpha)))));

    m_W_inc = (int)((m_W_inc_max / std::exp(alpha * congestionEstimation)) +
                    (beta * congestionEstimation) + gamma);

    
}

void
TcpAdaptiveReno::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked > 0)
    {
        EstimateIncWnd(tcb);
        // base_window = USE NEW RENO IMPLEMENTATION
        double adder =
            static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();
        adder = std::max(1.0, adder);
        m_W_base += static_cast<uint32_t>(adder);

        // change probe window..........................................
        m_W_probe = std::max((double)(m_W_probe + m_W_inc / (int)tcb->m_cWnd.Get()), (double)0);

        NS_LOG_LOGIC("Before " << tcb->m_cWnd << " ; base " << m_W_base << " ; probe "
                               << m_W_probe);
        tcb->m_cWnd = m_W_base + m_W_probe;

        NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh);
    }
}

uint32_t
TcpAdaptiveReno::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    m_rtt_prev_cong = m_rtt_cong; 
    m_jPacketLRtt =
        m_rtt_current; 

    double congestionEstimation = EstimateCongestionLevel();
   
    uint32_t ssthresh =
        std::max(2 * tcb->m_segmentSize, (uint32_t)(tcb->m_cWnd / (1.0 + congestionEstimation)));

   
    m_W_base = ssthresh;
    m_W_probe = 0;

    
    return ssthresh;
}

Ptr<TcpCongestionOps>
TcpAdaptiveReno::Fork()
{
    return CreateObject<TcpAdaptiveReno>(*this);
}

} // namespace ns3
