set terminal pngcairo size 1400,750 enhanced
set output "/home/phu/embedded_lab3/logs/latency_histogram.png"

set title "LAB-03: Cyclictest Latency Histogram - Standard Kernel"
set xlabel "Latency (microseconds)"
set ylabel "Number of samples"

set xrange [0:100]
set grid
set key top right

plot "/home/phu/embedded_lab3/logs/hist_standard.txt" using 1:2 with lines linewidth 2 title "Thread 0",      "" using 1:3 with lines linewidth 2 title "Thread 1",      "" using 1:4 with lines linewidth 2 title "Thread 2",      "" using 1:5 with lines linewidth 2 title "Thread 3"
