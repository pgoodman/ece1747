set datafile separator ","
set term png
set output out_file
set key invert reverse Left outside
set yrange [0:100]
set ylabel "% of total"
unset ytics
set grid y
set border 3
set style data histograms
set style histogram rowstacked
set style fill solid border -1
set boxwidth 0.75

plot in_file using (100.*$2):xtic(1) title column(2), \
     in_file using (100.*$3):xtic(1) title column(3), \
     in_file using (100.*$4):xtic(1) title column(4), \
     in_file using (100.*$5):xtic(1) title column(5), \
     in_file using (100.*$6):xtic(1) title column(6), \
     in_file using (100.*$7):xtic(1) title column(7)
