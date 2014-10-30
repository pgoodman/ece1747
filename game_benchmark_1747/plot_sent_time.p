set datafile separator ","

# Output file type and location
set term png
set output out_file

# Legend location
set key default
set key top left

# Size ratio
set terminal png size 1000,400
set size ratio 0.3

# Axis scaling
set yrange [0:500]
set ytics 50
set xrange [0:duration]
set xtics 100

unset log                              # remove any log-scaling
unset label                            # remove any previous labels

set title "Average Sending Time per Thread"
set xlabel "Time (iterations)"
set ylabel "Average Sending Time (ms)"

# Add in a rectangle behind the lines that highlights the times of the quest
set obj 1 rectangle from qbegin,graph 0 to qend,graph 1 fc lt -1 fs solid 0.15 noborder behind

set datafile missing "?"

# Plot columns 2, 3, 4, and 5 on the y axis for each X (column 1).
plot "sent_time.dat" using 1:2 title 'Thread 1' smooth csplines linecolor rgb "red", \
     "sent_time.dat" using 1:3 title 'Thread 2' smooth csplines linecolor rgb "green", \
     "sent_time.dat" using 1:4 title 'Thread 3' smooth csplines linecolor rgb "blue", \
     "sent_time.dat" using 1:5 title 'Thread 4' smooth csplines linecolor rgb "orange", \
     "quests.dat" using 1:2 notitle with lines lw 4 linecolor rgb "red", \
     "quests.dat" using 1:3 notitle with lines lw 4 linecolor rgb "green", \
     "quests.dat" using 1:4 notitle with lines lw 4 linecolor rgb "blue", \
     "quests.dat" using 1:5 notitle with lines lw 4 linecolor rgb "orange"