./kill_lttng.sh
lttng create "$1-4t-50rui-10ol-1000p"
lttng enable-event -a -u
lttng add-context -u -t vtid
lttng start
./server config_$1.ini 12345
lttng stop
#python3 stats.py ~/lttng-traces/*/ust/uid/1000/64-bit/
#gnuplot -e "out_file='$1_num_req.png'" plot_num_req.p

