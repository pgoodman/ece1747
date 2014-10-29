#first argument is path to trace lightest
#second argument is path to trace spread
#third argument is path to trace static
# Why? Alphabetical order that's why!
python3 stats.py $1
sh ./generate_graphs.sh lightest
python3 stats.py $2
sh ./generate_graphs.sh spread
python3 stats.py $3
sh ./generate_graphs.sh static
