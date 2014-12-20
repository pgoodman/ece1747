#! /bin/sh
gnuplot -e "out_file='/tmp/$1_num_req.png'" -e "duration=1500" -e "qbegin=3000" -e "qend=13000" plot_num_req.p
#gnuplot -e "out_file='/tmp/$1_num_updates.png'" -e "duration=1500" -e "qbegin=3000" -e "qend=13000" plot_num_updates.p
gnuplot -e "out_file='/tmp/$1_proc_time.png'" -e "duration=1500" -e "qbegin=3000" -e "qend=13000" plot_proc_time.p
#gnuplot -e "out_file='/tmp/$1_send_time.png'" -e "duration=1500" -e "qbegin=3000" -e "qend=13000" plot_sent_time.p
gnuplot -e "out_file='/tmp/$1_stage_breakdown_$2.png'" -e "duration=1500" -e "qbegin=3000" -e "qend=13000" -e "in_file='/tmp/stage_breakdown_$2.dat'" plot_stage_breakdown.p
