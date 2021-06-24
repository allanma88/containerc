# ip link set dev lo up
# ip link add veth0 type veth peer name veth1
# ip link set veth0 netns 1
# ifconfig veth1 10.1.1.1/24 up
# nsenter -t 1 -n ifconfig veth0 10.1.1.2/24 up
touch /mnt/d/OpenSource/containerc/src/test