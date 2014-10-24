set datafile separator ","

# Output file type and location
set term png
set output out_file

# Legend location
set key default
set key bottom right

# Axis scaling
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels

set title "Requests per Thread"
set xlabel "Time"
set ylabel "Number of Requests"

# Add in a rectangle behind the lines that highlights the times of the quest
set obj 1 rectangle from "300",graph 0 to "700",graph 1 fc lt -1 fs solid 0.15 noborder behind

set datafile missing "?"

# Plot columns 2, 3, 4, and 5 on the y axis for each X (column 1).
plot "num_req.dat" using 1:2 title 'Thread 1' smooth csplines linecolor rgb "red", \
     "num_req.dat" using 1:3 title 'Thread 2' smooth csplines linecolor rgb "green", \
     "num_req.dat" using 1:4 title 'Thread 3' smooth csplines linecolor rgb "blue", \
     "num_req.dat" using 1:5 title 'Thread 4' smooth csplines linecolor rgb "orange", \
     "quests.dat" using 1:2 notitle with lines lw 4 linecolor rgb "red", \
     "quests.dat" using 1:3 notitle with lines lw 4 linecolor rgb "green", \
     "quests.dat" using 1:4 notitle with lines lw 4 linecolor rgb "blue", \
     "quests.dat" using 1:5 notitle with lines lw 4 linecolor rgb "orange"
