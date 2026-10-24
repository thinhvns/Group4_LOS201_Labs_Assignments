set terminal pngcairo size 1400,800 enhanced
set output "latency_comparison.png"

set title "LAB-03: Cyclictest Latency - Normal vs Stress"
set xlabel "Latency (microseconds)"
set ylabel "Number of samples (log scale)"

set xrange [0:1000]
set logscale y
set yrange [1:*]
set grid
set key top right

plot "hist_standard.txt" using 1:($2+$3+$4+$5) \
     with impulses linewidth 2 title "Normal", \
     "hist_stress_full.txt" using 1:($2+$3+$4+$5) \
     with impulses linewidth 2 title "CPU Stress"
