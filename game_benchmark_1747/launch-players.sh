host="127.0.0.1"
port="12345"
echo "Launching $1 players to connect at $host:$port" 
for i in $(seq 1 $1);
do
  ./client $host":"$port > /dev/null 2>&1 &
done
