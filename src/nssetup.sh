# NETWORK INTERFACE

Container_Ip=10.1.1.2/16 # need an dynamic container ip
Container_BrIp=10.1.255.255
Container_Root=$ContainerBase/rootfs
VETHNAME=veth$ChildPid

result=$(ip link show br0 &> /dev/null)
if [ -z "$result" ]; then
    ip link add br0 type bridge
    ip addr add 10.1.1.1/16 broadcast $Container_BrIp dev br0
    ip link set dev br0 up
fi

ip link add $VETHNAME type veth peer name veth0
ip link set veth0 master br0
ip link set $VETHNAME master br0

ip link set dev $VETHNAME up

ip link set dev veth0 netns $ChildPid
nsenter -t $ChildPid -n ip link set dev veth0 name eth0
nsenter -t $ChildPid -n ip addr add $Container_Ip broadcast $Container_BrIp dev eth0
nsenter -t $ChildPid -n ip link set dev eth0 up
nsenter -t $ChildPid -n ip link set dev lo up

nsenter -t $ChildPid -n ip route add default via 10.1.1.1 dev eth0

# DNS
cp /etc/resolv.conf $Container_Root/etc/resolv.conf

# IPTABLE RULES

# Configuration options.

Container_CIDR=10.1.0.0/16
IPTABLES="/usr/sbin/iptables"

# /proc set up.

echo "1" > /proc/sys/net/ipv4/ip_forward

# rules set up.

## raw table

### PREROUTING chain

$IPTABLES -t raw -A PREROUTING -p ALL -s $Container_CIDR -j ACCEPT

## nat table

### PREROUTING chain

# $IPTABLES -t nat -A PREROUTING -d $Container_CIDR --dport $DestPort -j DNAT --to-destination $Container_CIDR

### POSTROUTING chain
$IPTABLES -t nat -A POSTROUTING -s $Container_CIDR -j MASQUERADE