sh kill_lttng.sh
lttng create "$1-$2t-$3c-64x64-100rui-run$4-$5"
lttng enable-event -a -u
lttng add-context -t vtid -u
lttng start
cgexec -g cpuset:simmud-server ./server config_$1.ini 12345
lttng stop

