#!/bin/bash

cd ..

# Start of the loop
for i in 20 40 60 80 100
do

    ./ns3 run "scratch/1905064_2.cc --Nodes=$i --datFileType=1"
done

for i in 10 20 30 40 50
do

    ./ns3 run "scratch/1905064_2.cc --Flows=$i --datFileType=2"
done

for i in 100 200 300 400 500
do

    ./ns3 run "scratch/1905064_2.cc --Packets=$i --datFileType=3"
done

for i in 5 10 15 20 25
do

    ./ns3 run "scratch/1905064_2.cc --Speed=$i --datFileType=4"
done



# List of data files
dat_files=("mobile_Nodes_vs_Throughput.dat" "mobile_Nodes_vs_DeliveryRatio.dat" "mobile_Flows_vs_Throughput.dat" "mobile_Flows_vs_DeliveryRatio.dat" "mobile_Packets_vs_Throughput.dat" "mobile_Packets_vs_DeliveryRatio.dat" "mobile_Speed_vs_Throughput.dat" "mobile_Speed_vs_DeliveryRatio.dat") 
 
cd scratch
mkdir outputs_mobile
mkdir datFiles_mobile
cd ..

# Loop over each data file and generate graphs
for dat_file in "${dat_files[@]}"; do
    string="${dat_file}"
    delimiter=","
    IFS="$delimiter"
    array=($string)
    IFS=" "
    gnuplot <<- EOF
    set terminal png
    set output 'scratch/outputs_mobile/output_graph_${dat_file}.png'

    set datafile separator "\t"
    set title "mobile Data Plot"
    set xlabel "${string[1]}"
    set ylabel "${string[3]}"

    plot "${dat_file}" using 1:2 with lines
EOF
done

for dat_file in "${dat_files[@]}"; do
    mv "${dat_file}" "scratch/datFiles_mobile/${dat_file}"

done

