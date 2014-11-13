host1="pag@cs.toronto.edu"
host2="mccrae.syslab.sandbox"

echo "Tunneling sent to localhost:12345 to $host1 and sent $host2"
ssh -L 12345:$host2:12345 -N $host1


