#! /bin/sh
gnuplot -e "out_file='/tmp/$1_num_req.png'" -e "duration=2000" -e "qbegin=300" -e "qend=1300" plot_num_req.p
gnuplot -e "out_file='/tmp/$1_num_updates.png'" -e "duration=2000" -e "qbegin=300" -e "qend=1300" plot_num_updates.p
gnuplot -e "out_file='/tmp/$1_proc_time.png'" -e "duration=2000" -e "qbegin=300" -e "qend=1300" plot_proc_time.p
gnuplot -e "out_file='/tmp/$1_send_time.png'" -e "duration=2000" -e "qbegin=300" -e "qend=1300" plot_sent_time.p
gnuplot -e "out_file='/tmp/$1_stage_breakdown.png'" -e "duration=2000" -e "qbegin=300" -e "qend=1300" plot_stage_breakdown.p
