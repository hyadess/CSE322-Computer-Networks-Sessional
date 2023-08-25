/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include <fstream>
#include "ns3/queue.h"


/*


    n2  --*                                 *-- n7

    n3  --*     AP                  AP      *-- n8
                        10.1.1.0
    n4  --*     n0 ---------------- n1      *-- n9
                    point-to-point
    n5  --*                                 *-- n10

    n6  --*                                 *-- n11

wifi 10.1.2.0                              wifi 10.1.3.0

The nodes in the left network are senders and the right ones are receivers.


*/


using namespace ns3;

/*Define a Log component with a specific name.

This macro should be used at the top of every file in which you want to use 
the NS_LOG macro. This macro defines a new "log component" which can be later 
selectively enabled or disabled with the ns3::LogComponentEnable and 
ns3::LogComponentDisable functions or with the NS_LOG environment variable.*/

NS_LOG_COMPONENT_DEFINE("1905064_2");


Ptr<PacketSink> sink;     


uint64_t transmitted_packets=0;
uint64_t received_packets=0;

void TxTraceCallback (Ptr<const Packet> packet) {
  transmitted_packets+=packet->GetSize();
  //std::cout<<packet->GetSize()<<std::endl;
}
void
AddressTracedCallback(Ptr< const Packet > packet, const Address &address){
  
  received_packets+=packet->GetSize();
  //std::cout<<packet->GetSize()<<std::endl;
}






int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);

    // ========================== variables ==========================
    std::string gatewayBandwidth= "1Mbps";
    std::string gatewayDelay = "5ms";

    uint32_t Nodes = 20;
    uint32_t Flows = 100;
    uint32_t PacketsPerSecond = 500;
    uint32_t speed=5;
    
    

    uint32_t datType=-1;


    uint32_t payloadSize = 1024;           /* Transport layer payload size in bytes. */
    
    double simulationTime = 10;            /* Simulation time in seconds. */


    // ========================== variables inputs ==========================

    cmd.AddValue("Nodes","Number of nodes sink and sender nodes",Nodes);
    cmd.AddValue("Flows","Number of Flows",Flows);
    cmd.AddValue("Packets","Number of packets per second",PacketsPerSecond);
    cmd.AddValue("Speed","node speed in the networks",speed);
    cmd.AddValue("datFileType","for variable vs performance metrics graphs.--1 for Nodes, 2 for Flows, 3 for Packets, 4 for speed",datType);
    cmd.Parse(argc, argv);


    
    uint32_t bitRate=8*1024*PacketsPerSecond;
    std::string dataRate = std::to_string(bitRate)+"bps";      /* Application layer datarate. */
    ///check if everything is ok......................

    std::string infos="variables-";
    infos+="-Nodes-"+std::to_string(Nodes);
    infos+="--Flows-"+std::to_string(Flows);
    infos+="--DataRate-"+std::to_string(bitRate)+"bps";
    infos+="--Speed-"+std::to_string(speed);

    std::cout<<infos<<std::endl;


    //************************************configs***********************************
    //LogComponentEnableAll (LOG_LEVEL_INFO);
    LogComponentEnable ("TcpSocketBase", LOG_ERROR);
    LogComponentEnable ("Ipv4GlobalRouting", LOG_ERROR);
    //LogComponentEnableAll (LOG_LEVEL_NONE);

    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));


    //  sets the time resolution to one nanosecond
    Time::SetResolution(Time::NS);

    // Add logging to code
    NS_LOG_INFO("Creating Topology");













    // gateway (only network layer)***********************************************************************************
    /// gives us bottleneckNodes, getwayDevices............
    NodeContainer bottleneckNodes;
    bottleneckNodes.Create(2);

    PointToPointHelper gateway;
    gateway.SetDeviceAttribute("DataRate", StringValue(gatewayBandwidth)); 
    gateway.SetChannelAttribute("Delay", StringValue(gatewayDelay));

    NetDeviceContainer gatewayDevices;
    gatewayDevices = gateway.Install(bottleneckNodes);

    //Wifi(physical layer--yansWifi,datalink layer--mac,network layer)****************************************************
    NodeContainer wifiSenderNodes;
    NodeContainer wifiReceiverNodes;
    NodeContainer wifiApSenderNode = bottleneckNodes.Get(0);
    NodeContainer wifiApReceiverNode = bottleneckNodes.Get(1);
    wifiReceiverNodes.Create(Nodes);
    wifiSenderNodes.Create(Nodes);

    YansWifiChannelHelper channelLeft = YansWifiChannelHelper::Default();
    YansWifiChannelHelper channelRight = YansWifiChannelHelper::Default();
    //channelLeft.AddPropagationLoss("ns3::RangePropagationLossModel","MaxRange",DoubleValue(coverageArea));
    //channelRight.AddPropagationLoss("ns3::RangePropagationLossModel","MaxRange",DoubleValue(coverageArea));
    YansWifiPhyHelper phyLeft, phyRight;
    phyLeft.SetChannel(channelLeft.Create());
    phyRight.SetChannel(channelRight.Create());
    //phyLeft.Set("TxPowerStart",DoubleValue(10*log10(tx_range*1.0)));
    //phyLeft.Set("TxPowerEnd",DoubleValue(10*log10(tx_range*1.0)));
    //phyRight.Set("TxPowerStart",DoubleValue(10*log10(tx_range*1.0)));
    //phyRight.Set("TxPowerEnd",DoubleValue(10*log10(tx_range*1.0)));


    WifiHelper wifi;
    //wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper macLeft, macRight;
    Ssid ssidLeft = Ssid("ns-3-ssid-left");
    Ssid ssidRight = Ssid("ns-3-ssid-right"); 

    NetDeviceContainer staLeftDevices, staRightDevices;
    macLeft.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidLeft), "ActiveProbing", BooleanValue(false));
    macRight.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssidRight), "ActiveProbing", BooleanValue(false));
    staLeftDevices = wifi.Install(phyLeft, macLeft, wifiSenderNodes);
    staRightDevices = wifi.Install(phyRight, macRight, wifiReceiverNodes);

    NetDeviceContainer apLeftDevices, apRightDevices;
    macLeft.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidLeft));
    macRight.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssidRight));
    apLeftDevices = wifi.Install(phyLeft, macLeft, wifiApSenderNode);
    apRightDevices = wifi.Install(phyRight, macRight, wifiApReceiverNode);

    MobilityHelper mobility;

     mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (0.5),
                                 "DeltaY", DoubleValue (1.0),
                                 "GridWidth", UintegerValue (50),
                                 "LayoutType", StringValue ("RowFirst"));
    // all nodes are mobile
    //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant="+std::to_string(speed)+"]"));
    mobility.Install(wifiReceiverNodes);
    mobility.Install(wifiSenderNodes);
    mobility.Install(wifiApReceiverNode);
    mobility.Install(wifiApSenderNode);


    // Internet Stack *********************************************************************
    InternetStackHelper stack;
    stack.Install(wifiReceiverNodes);
    stack.Install(wifiSenderNodes);
    stack.Install(wifiApReceiverNode);
    stack.Install(wifiApSenderNode);

    //  IP Addresses**********************************************************
    Ipv4AddressHelper addressgateway;
    addressgateway.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer gatewayInterfaces;
    gatewayInterfaces = addressgateway.Assign(gatewayDevices);

    Ipv4AddressHelper addressLeft;
    addressLeft.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer staLeftInterfaces,apLeftInterfaces;
    staLeftInterfaces = addressLeft.Assign(staLeftDevices);
    apLeftInterfaces = addressLeft.Assign(apLeftDevices);

    Ipv4AddressHelper addressRight;
    addressRight.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staRightInterfaces,apRightInterfaces;
    staRightInterfaces = addressRight.Assign(staRightDevices);
    apRightInterfaces = addressRight.Assign(apRightDevices);

    







    
    // Applications**********************************************************************


    
  
    // receiver
    uint16_t port = 9;
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

    ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < Nodes; ++i)
    {
      // create sink app on left side node
    //   Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port+i));
    //   PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      sinkApps.Add (packetSinkHelper.Install (wifiReceiverNodes.Get(i)));
    }
    
    

    // sender

    
    //std::cout<<"start"<<std::endl;
    
    
    // ApplicationContainer senderApps;

    // for (uint32_t i = 0; i < Flows; i++)
    // {
    //     uint32_t index = i % Nodes;
    //     OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(staRightInterfaces.GetAddress(index), port));
    //     onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    //     onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    //     onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    //     senderApps.Add(onOffHelper.Install(wifiSenderNodes.Get(index)));
    // }
    //std::cout<<"done"<<std::endl;





    OnOffHelper senderHelper("ns3::TcpSocketFactory", Address());
    senderHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    senderHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));


    ApplicationContainer senderApps;
    int cur_flow_count = 0;
    for (uint32_t i = 0; i < Nodes; ++i)
    {
      // Create an on/off app on right side node which sends packets to the left side
      AddressValue remoteAddress (InetSocketAddress (staRightInterfaces.GetAddress(i), port));
      
      for(uint32_t j = 0; j < Nodes; ++j)
      {
        senderHelper.SetAttribute ("Remote", remoteAddress);
        senderApps.Add (senderHelper.Install (wifiSenderNodes.Get(j)));

        cur_flow_count++;
        if(cur_flow_count >= Flows)
          break;
      }

      if(cur_flow_count >= Flows)
          break;

      // clientHelper.SetAttribute ("Remote", remoteAddress);
      // clientApps.Add (clientHelper.Install (wifiStaNodesRight.Get(i)));
    }












    sinkApps.Start(Seconds(1.0));
    senderApps.Start(Seconds(2.0));

    senderApps.Stop(Seconds(6.0));
    sinkApps.Stop(Seconds(7.0));



    //Routing ********************************************************************************************************
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();





    /// simulation and performance metrics***************************************************************************

    //Enable packet tracing for packet delivery ratio..................

    // Connect trace source for packet transmission events
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx", MakeCallback(&TxTraceCallback));
    // for receiving event.........
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&AddressTracedCallback));

 

    
                                

    //  looking through the list of scheduled events and executing them
    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();



    //calculate throughput..................

    double totalThroughput=0;

    for (uint32_t i = 0; i < Nodes; ++i)
    {
      sink = StaticCast<PacketSink>(sinkApps.Get(i));
      totalThroughput+= (sink->GetTotalRx() * 8) ;
    }
    double averageThroughput=totalThroughput/(simulationTime);

  
    Simulator::Destroy(); // destroy all objects created



    /// the graph part************************************************************************************


    if(datType==1)
    {
      std::ofstream dataFile("mobile_Nodes_vs_Throughput.dat", std::ios::app);
      dataFile << Nodes << "\t" << averageThroughput << "\n";
      dataFile.close();

      std::ofstream dataFile2("mobile_Nodes_vs_DeliveryRatio.dat", std::ios::app);
      dataFile2 << Nodes << "\t" <<  1.0*received_packets/transmitted_packets<< "\n";
      dataFile2.close();
    }

    else if(datType==2)
    {
      std::ofstream dataFile("mobile_Flows_vs_Throughput.dat", std::ios::app);
      dataFile << Flows << "\t" << averageThroughput << "\n";
      dataFile.close();

      std::ofstream dataFile2("mobile_Flows_vs_DeliveryRatio.dat", std::ios::app);
      dataFile2 << Flows << "\t" <<  1.0*received_packets/transmitted_packets<< "\n";
      dataFile2.close();
    }
    else if(datType==3)
    {
      std::ofstream dataFile("mobile_Packets_vs_Throughput.dat", std::ios::app);
      dataFile << PacketsPerSecond << "\t" << averageThroughput << "\n";
      dataFile.close();

      std::ofstream dataFile2("mobile_Packets_vs_DeliveryRatio.dat", std::ios::app);
      dataFile2 << PacketsPerSecond << "\t" <<  1.0*received_packets/transmitted_packets<< "\n";
      dataFile2.close();
    }
    else if(datType==4)
    {
      std::ofstream dataFile("mobile_Speed_vs_Throughput.dat", std::ios::app);
      dataFile << speed << "\t" << averageThroughput << "\n";
      dataFile.close();

      std::ofstream dataFile2("mobile_Speed_vs_DeliveryRatio.dat", std::ios::app);
      dataFile2 << speed << "\t" <<  1.0*received_packets/transmitted_packets<< "\n";
      dataFile2.close();
    }


    NS_LOG_UNCOND("average Throughput: " << averageThroughput<< " bps");
    NS_LOG_UNCOND("Delivery Ratio: " << 1.0*received_packets/transmitted_packets);
    return 0;
}


