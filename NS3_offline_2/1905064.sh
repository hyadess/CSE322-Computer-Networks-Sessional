#!/bin/bash
gnu_file1="pltFiles/cwnd.plt"
gnu_file2="pltFiles/data.plt"
gnu_file3="pltFiles/extra.plt"

mkdir -p output

makeFile()
{
    mkdir -p output/"$1"
    rm -f output/"$1"/data_1.txt
    touch output/"$1"/data_1.txt
    rm -f output/"$1"/data_2.txt
    touch output/"$1"/data_2.txt
    rm -f output/"$1"/"$1".txt
}

# first arg selects method, second argument selects no of nodes
if [ "$1" == 1 ]; then
    tcp="ns3::TcpHighSpeed"
    method="tcpHighSpeed"
elif [ "$1" == 2 ]; then
    tcp="ns3::TcpAdaptiveReno"
    method="adaptiveReno"
else [ "$1" == 3 ]
    tcp="ns3::TcpWestwoodPlus"
    method="westwoodPlus"
fi
makeFile $method
path="output"/"$method"
ns3Path="scratch"/"output"/"$method"


arr=("1" "5" "10" "20" "50" "100" "300")
for i in "${arr[@]}";
do
    cd ..
    ./ns3 run "scratch/1905064.cc --tcp2=${tcp} --bottleneckDataRate=${i} --output_folder=${ns3Path} --exp=1" >> "${ns3Path}/${method}.txt" 2>&1
    cd scratch
  
    if [ $i -eq 50 ]; then
        gnuplot -c $gnu_file1 "$path/cwnd_bottleneckDataRate_${i}.png" "${path}/flow1.cwnd" 1 2 "newreno" "${path}/flow2.cwnd" 2 ${method}
    fi
    echo "bottleneckDataRate $i done"
done


gnuplot -c $gnu_file2 "$path/throughput vs bottleneckDataRate.png" "Throughput VS Bottleneck Link Capacity" "Bottleneck Link Capacity (Mbps)" "$path/data_1.txt" 1 3 4 ${method}
gnuplot -c $gnu_file3 "$path/fairness index vs bottleneckDataRate.png" "Fairness Index VS Bottleneck Link Capacity" "Bottleneck Link Capacity (Mbps)" "Fairness Index" "$path/data_1.txt" 1 5 "newreno + ${method}"

for i in {2..6}
do
    cd ..
    ./ns3 run "scratch/1905064.cc --tcp2=${tcp} --packetLossRate=${i} --output_folder=${ns3Path} --exp=2">> "${ns3Path}/${method}.txt" 2>&1
    cd scratch
    echo "loss rate exp -$i done"
done



gnuplot -c $gnu_file2 "$path/throughput vs packet loss rate.png" "Throughput VS Packet Loss Rate" "Packet Loss Rate (%) Exponent" "$path/data_2.txt" 2 3 4 ${method}
gnuplot -c $gnu_file3 "$path/fairness index vs packet loss rate.png" "Fairness Index VS Packet Loss Rate" "Packet Loss Rate (%) Exponent" "Fairness Index" "$path/data_2.txt" 2 5 "newreno + ${method}"
