set terminal png
set key box
set key width 1
set key font "Arial,14"
set style line 1 lt rgb "cyan" lw 3 pt 6
set grid
set border 3
set tics nomirror
set output ARG1
set title "Congestion Window vs Time"
set xlabel "Time (s)"
set ylabel "Cwnd (bytes)"
plot ARG2 using int(ARG3):int(ARG4)  with linespoints title ARG5, ARG6 using int(ARG3):int(ARG7) with linespoints title ARG8