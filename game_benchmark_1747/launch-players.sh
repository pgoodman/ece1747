host="127.0.0.1"
port="12345"
echo "Launching $2 players to connect at $1:$port" 
for i in $(seq 1 $2);
do
  ./client $1":"$port > /dev/null 2>&1 &
done
