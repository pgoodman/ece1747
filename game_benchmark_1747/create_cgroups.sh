#have to be root
#!/bin/bash
set -x

USERNAME= $SUDO_USER
SERVER_CPUS=2
CLIENT_CPUS=2
#cgroup for server
sudo cgcreate -t $USERNAME -a $USERNAME -g cpuset:simmud-server
sudo cgset -r cpuset.cpus="0-$((SERVER_CPUS-1))" simmud-server
#cgroup for clients
sudo cgcreate -t $USERNAME -a $USERNAME -g cpuset:simmud-clients
sudo cgset -r cpuset.cpus="$((SERVER_CPUS))-$((SERVER_CPUS+CLIENT_CPUS-1))" simmud-clients
