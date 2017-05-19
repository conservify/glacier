set datafile separator ","
set terminal png size 900,400
set title "Battery"
set xdata time
set timefmt "%s"
set grid
set key left top

plot "data.csv" using 1:2 with lines lw 2 lt 3
