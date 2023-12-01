#ifndef TCP_ADAPTIVERENO_H
#define TCP_ADAPTIVERENO_H

#include "tcp-congestion-ops.h"
#include "tcp-westwood-plus.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"
#include "ns3/event-id.h"

namespace ns3 {

class Packet;
class TcpHeader;
class Time;
class EventId;

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP ADAPTIVE RENO.
 *
 */
class TcpAdaptiveReno : public TcpWestwoodPlus
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpAdaptiveReno (void);
  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpAdaptiveReno (const TcpAdaptiveReno& sock);
  virtual ~TcpAdaptiveReno (void);

  /**
   * \brief Filter type (None or Tustin)
   */
  enum FilterType 
  {
    NONE,
    TUSTIN
  };

  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);

  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                          const Time& rtt);

  virtual Ptr<TcpCongestionOps> Fork ();

private:
  // void UpdateAckedSegments (int acked);

  

  double EstimateCongestionLevel();

  void EstimateIncWnd(Ptr<TcpSocketState> tcb);

protected:
  void EstimateBW (const Time& rtt, Ptr<TcpSocketState> tcb);
  virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  Time                   m_rtt_min;                 //!< Minimum RTT
  Time                   m_rtt_current;             //!< Current RTT
  Time                   m_jPacketLRtt;            //!< RTT of j packet loss
  Time                   m_rtt_cong;                //!< Conjestion RTT (j th event)
  Time                   m_rtt_prev_cong;            //!< Previous Conjestion RTT (j-1 th event)

  // Window calculations
  int32_t                m_W_inc;                 //!< Increment Window
  uint32_t               m_W_base;                //!< Base Window
  int32_t                m_W_probe;               //!< Probe Window 
};

} // namespace ns3

#endif /* TCP_ADAPTIVE_RENO_H */
