set datafile separator ","
set terminal png size 900,400
set title "Voltage"
set ylabel "Level"
set xlabel "Date"
set xdata time
set timefmt "%s"
set format x "%m/%d %H"
set ytics font "Verdana,8" 
set xtics font "Verdana,8" 
set key left top
set grid

plot "morningstar.csv" using 1:6 with lines lw 2 lt 3 title 'battery voltage'
