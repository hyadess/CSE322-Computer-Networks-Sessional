
#include "ns3/applications-module.h"
#include "ns3/callback.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/stats-module.h"

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("1905064");

class SimulatorApp : public Application
{
  public:
    SimulatorApp();
    virtual ~SimulatorApp();

    
    static TypeId GetTypeId(void);
    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               DataRate dataRate,
               uint32_t simultime);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
    uint32_t m_simultime;
};

SimulatorApp::SimulatorApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0),
      m_simultime(0)
{
}

SimulatorApp::~SimulatorApp()
{
    m_socket = 0;
}

/* static */
TypeId
SimulatorApp::GetTypeId(void)
{
    static TypeId tid = TypeId("SimulatorApp")
                            .SetParent<Application>()
                            .SetGroupName("Tutorial")
                            .AddConstructor<SimulatorApp>();
    return tid;
}

void
SimulatorApp::Setup(Ptr<Socket> socket,
                    Address address,
                    uint32_t packetSize,
                    DataRate dataRate,
                    uint32_t simultime)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_dataRate = dataRate;
    m_simultime = simultime;
  
}

void
SimulatorApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        m_socket->Bind();
    }
    else
    {
        m_socket->Bind6();
    }
    m_socket->Connect(m_peer);
    SendPacket();
}

void
SimulatorApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
SimulatorApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (Simulator::Now().GetSeconds() < m_simultime)
        ScheduleTx();
}

void
SimulatorApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &SimulatorApp::SendPacket, this);
    }
}

static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    // NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << " " << newCwnd);
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newCwnd << std::endl;
}

int
main(int argc, char* argv[])
{
    uint32_t payloadSize = 1200;
    std::cout << "hey\n";

    //************define variables and constants.....................
    int leafNodes = 2;
    int totalFlows = 2;
    int bottleneckDataRate = 50;
    int bottleneckDelay = 100;
    int lossExponential = 6;
    std::string leafDataRate = "1Gbps";
    std::string leafDelay = "1ms";
    int simulationTime = 60;
    int cleanupTime = 2;
    int exp = 1;

    //***********************different tcps and outputs................
    std::string tcpType2 = "ns3::TcpHighSpeed"; 
    std::string output_folder = "task1/tcphs";

    // ...................................input from CMD
    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp2", "TCP 2", tcpType2);
    cmd.AddValue("exp", "1 for bottleneck graph, 2 for loss rate graph", exp);
    cmd.AddValue("output_folder", "data txt file", output_folder);
    cmd.AddValue("bottleneckDataRate",
                 "Max Packets allowed",
                 bottleneckDataRate);
    cmd.AddValue("packetLossRate", "loss rate ( x for 10^-x)", lossExponential);
   
    
    cmd.Parse(argc, argv);

    //**************************************set all the variables................
    std::string outFile = output_folder + "/data_" + std::to_string(exp) + ".txt";
    totalFlows = leafNodes;
    std::string bottleNeckDataRate = std::to_string(bottleneckDataRate) + "Mbps";
    std::string bottleNeckDelay = std::to_string(bottleneckDelay) + "ms";
    double packetLossRate = (1.0 / std::pow(10, lossExponential));
    

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));

    // ************************************************SETUP NODE AND DEVICE


    PointToPointHelper senderReceiverNodes;
    senderReceiverNodes.SetDeviceAttribute("DataRate", StringValue(leafDataRate));
    senderReceiverNodes.SetChannelAttribute("Delay", StringValue(leafDelay));

    senderReceiverNodes.SetQueue(
        "ns3::DropTailQueue",
        "MaxSize",
        StringValue(std::to_string(bottleneckDelay * bottleneckDataRate) + "p"));

    PointToPointHelper bottleneckNodes;
    bottleneckNodes.SetDeviceAttribute("DataRate", StringValue(bottleNeckDataRate));
    bottleneckNodes.SetChannelAttribute("Delay", StringValue(bottleNeckDelay));

    

    


    PointToPointDumbbellHelper dumbbel(leafNodes,
                                       senderReceiverNodes,
                                       leafNodes,
                                       senderReceiverNodes,
                                       bottleneckNodes);

    // **********************adding error...................................
    
    Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
    errorModel->SetAttribute("ErrorRate", DoubleValue(packetLossRate));
    dumbbel.m_routerDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));


    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    InternetStackHelper stack1;

    stack1.Install(dumbbel.GetLeft(0));  // left leave
    stack1.Install(dumbbel.GetRight(0)); // right leave

    stack1.Install(dumbbel.GetLeft());  // left bottleneck with tcp new reno
    stack1.Install(dumbbel.GetRight()); // right bottleneck with tcp new reno


    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcpType2));
    InternetStackHelper stack2;

    stack2.Install(dumbbel.GetLeft(1));  // left leave
    stack2.Install(dumbbel.GetRight(1)); // right leave



    FlowMonitorHelper flowTracker;
    flowTracker.SetMonitorAttribute("MaxPerHopDelay", TimeValue(Seconds(cleanupTime)));
    Ptr<FlowMonitor> monitorPtr = flowTracker.InstallAll();

    // ASSIGN IP Addresses
    dumbbel.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),  // left nodes
                                Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),  // right nodes
                                Ipv4AddressHelper("10.3.1.0", "255.255.255.0")); // routers
    Ipv4GlobalRoutingHelper::PopulateRoutingTables(); // populate routing table

    // ****************************installing flow monitor...................
    

    // setup and start simulation**********************************************
    uint16_t port = 9;
    for (int i = 0; i < totalFlows; i++)
    {
        Address sinkAddress(InetSocketAddress(dumbbel.GetRightIpv4Address(i), port));
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                          InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = packetSinkHelper.Install(dumbbel.GetRight(i));
        sinkApps.Start(Seconds(0));
        sinkApps.Stop(Seconds(simulationTime + cleanupTime));

        Ptr<Socket> dumbbell_tcp_socket =
            Socket::CreateSocket(dumbbel.GetLeft(i), TcpSocketFactory::GetTypeId());

        Ptr<SimulatorApp> app = CreateObject<SimulatorApp>();
        app->Setup(dumbbell_tcp_socket,
                   sinkAddress,
                   payloadSize,
                   DataRate(leafDataRate),
                   simulationTime);
        dumbbel.GetLeft(i)->AddApplication(app);
        app->SetStartTime(Seconds(1));
        app->SetStopTime(Seconds(simulationTime));

        std::ostringstream oss;
        oss << output_folder << "/flow" << i + 1 << ".cwnd";
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream(oss.str());
        dumbbell_tcp_socket->TraceConnectWithoutContext("CongestionWindow",
                                                        MakeBoundCallback(&CwndChange, stream));
    }

    Simulator::Stop(Seconds(simulationTime + cleanupTime));
    Simulator::Run();

    
  
    uint32_t ReceivedPackets = 0;
    uint32_t LostPackets = 0;
    uint32_t SentPackets = 0;


    int cur = 0;
    float averageThroughput = 0;
    float currentThroughput = 0;
    float throughputArray[] = {0, 0};

    
    FlowMonitor::FlowStatsContainer stats = monitorPtr->GetFlowStats();

    double numerator = 0;
    double denominator = 0;
    uint32_t time=(simulationTime + cleanupTime) * 1000;
    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        

        currentThroughput = it->second.rxBytes * 8.0 / time;

        uint32_t pp,qq,rr;
        pp=it->second.txPackets;
        qq=it->second.rxPackets;
        rr=it->second.lostPackets;
        SentPackets += pp;
        ReceivedPackets += qq;
        LostPackets += rr;
        if (cur % 2 == 1)
        {
            throughputArray[1] += it->second.rxBytes;
        }
        else
        {
            throughputArray[0] += it->second.rxBytes;
        }
        

        cur++;

        numerator += currentThroughput;
        denominator += (currentThroughput * currentThroughput);
    }

    double index = (numerator * numerator) / (cur * denominator);
    throughputArray[0] /= time;
    throughputArray[1] /= time;
    averageThroughput = throughputArray[0] + throughputArray[1];
    std::ofstream MyFile(outFile, std::ios_base::app);
    
    MyFile << bottleNeckDataRate.substr(0, bottleNeckDataRate.length() - 4) << " "
           << -1 * lossExponential << " " << throughputArray[0] << " " << throughputArray[1] << " "
           << " " << index << std::endl;

    Simulator::Destroy();

    return 0;
}