#!/bin/bash

# declare array of IP Addresses
# declare array of namespaces
declare -a ips=("10.0.10.69" "10.0.10.70" "10.0.20.69" "10.0.20.70" "10.0.30.69" "10.0.30.70" "10.0.40.69" "10.0.40.70" "10.0.50.69" "10.0.50.70" "10.0.60.69" "10.0.60.70")
declare -a namespaces=("N1" "N2" "N3" "N4" "N5" "N6")

# obtain the number of IP Addresses and namespaces
ipsLength=${#ips[@]}
namespaceLength=${#namespaces[@]}

# function to create all the namespaces
createNamespaces() {
    local arr=("$@")
    length=${#arr[@]}
    for(( i=0; i<$length; i++))
    do 
        sudo ip netns add ${arr[$i]}
    done
}

# helper function to delete all the namespaces
delete() {
    local arr=("$@")
    length=${#arr[@]}
    for(( i=0; i<$length; i++))
    do 
        ip netns delete ${arr[$i]}
    done
}

# function to ping all the IP Addreses from a given namespace 
ping() {
    local arr=("$@")
    echo
    echo
    echo "######################################## Ping from ${arr[0]} ########################################"
    length=${#arr[@]}
    for(( i=1; i<$length; i++))
    do 
        sudo ip netns exec ${arr[0]} ping -c3 ${arr[$i]} 
    done
    echo
    echo
}

# create the namespaces
createNamespaces "${namespaces[@]}"

# create the three pairs of veth interfaces 
sudo ip link add V1 type veth peer name V2
sudo ip link add V3 type veth peer name V4
sudo ip link add V5 type veth peer name V6
sudo ip link add V7 type veth peer name V8
sudo ip link add V9 type veth peer name V10
sudo ip link add V11 type veth peer name V12

# assign the veth interfaces to the respective network namespaces
# assign as per the problem statement
sudo ip link set V1 netns N1
sudo ip link set V2 netns N2
sudo ip link set V3 netns N2
sudo ip link set V4 netns N3
sudo ip link set V5 netns N3
sudo ip link set V6 netns N4
sudo ip link set V7 netns N4
sudo ip link set V8 netns N5
sudo ip link set V9 netns N5
sudo ip link set V10 netns N6
sudo ip link set V11 netns N6
sudo ip link set V12 netns N1

# assign the corresponding IP address range to the veth interfaces
# assign as per the problem statement
sudo ip netns exec N1 ip addr add 10.0.10.69/24 dev V1
sudo ip netns exec N2 ip addr add 10.0.10.70/24 dev V2
sudo ip netns exec N2 ip addr add 10.0.20.69/24 dev V3
sudo ip netns exec N3 ip addr add 10.0.20.70/24 dev V4
sudo ip netns exec N3 ip addr add 10.0.30.69/24 dev V5
sudo ip netns exec N4 ip addr add 10.0.30.70/24 dev V6
sudo ip netns exec N4 ip addr add 10.0.40.69/24 dev V7
sudo ip netns exec N5 ip addr add 10.0.40.70/24 dev V8
sudo ip netns exec N5 ip addr add 10.0.50.69/24 dev V9
sudo ip netns exec N6 ip addr add 10.0.50.70/24 dev V10
sudo ip netns exec N6 ip addr add 10.0.60.69/24 dev V11
sudo ip netns exec N1 ip addr add 10.0.60.70/24 dev V12

# bring up the veth interfaces
sudo ip netns exec N1 ip link set V1 up
sudo ip netns exec N2 ip link set V2 up
sudo ip netns exec N2 ip link set V3 up
sudo ip netns exec N3 ip link set V4 up
sudo ip netns exec N3 ip link set V5 up
sudo ip netns exec N4 ip link set V6 up
sudo ip netns exec N4 ip link set V7 up
sudo ip netns exec N5 ip link set V8 up
sudo ip netns exec N5 ip link set V9 up
sudo ip netns exec N6 ip link set V10 up
sudo ip netns exec N6 ip link set V11 up
sudo ip netns exec N1 ip link set V12 up

# bring up the lo interfaces for each network namespace
# sanity check
sudo ip netns exec N1 ip link set lo up
sudo ip netns exec N2 ip link set lo up
sudo ip netns exec N3 ip link set lo up
sudo ip netns exec N4 ip link set lo up
sudo ip netns exec N5 ip link set lo up
sudo ip netns exec N6 ip link set lo up

# populate the routing tables for each namespace
# go through each namespace and find the routes to be added (ensure clockwise pinging only)
# ensure there is a return route also to each added routes

# add for N1
sudo ip netns exec N1 ip route add 10.0.20.0/24 via 10.0.10.70 dev V1
sudo ip netns exec N1 ip route add 10.0.30.0/24 via 10.0.10.70 dev V1
sudo ip netns exec N1 ip route add 10.0.40.0/24 via 10.0.10.70 dev V1
sudo ip netns exec N1 ip route add 10.0.50.0/24 via 10.0.10.70 dev V1

# add for N2
sudo ip netns exec N2 ip route add 10.0.30.0/24 via 10.0.20.70 dev V3
sudo ip netns exec N2 ip route add 10.0.40.0/24 via 10.0.20.70 dev V3
sudo ip netns exec N2 ip route add 10.0.50.0/24 via 10.0.20.70 dev V3
sudo ip netns exec N2 ip route add 10.0.60.0/24 via 10.0.20.70 dev V3

# add for N3
sudo ip netns exec N3 ip route add 10.0.10.0/24 via 10.0.30.70 dev V5
sudo ip netns exec N3 ip route add 10.0.40.0/24 via 10.0.30.70 dev V5
sudo ip netns exec N3 ip route add 10.0.50.0/24 via 10.0.30.70 dev V5
sudo ip netns exec N3 ip route add 10.0.60.0/24 via 10.0.30.70 dev V5

# add for N4
sudo ip netns exec N4 ip route add 10.0.10.0/24 via 10.0.40.70 dev V7
sudo ip netns exec N4 ip route add 10.0.20.0/24 via 10.0.40.70 dev V7
sudo ip netns exec N4 ip route add 10.0.50.0/24 via 10.0.40.70 dev V7
sudo ip netns exec N4 ip route add 10.0.60.0/24 via 10.0.40.70 dev V7

# add for N5
sudo ip netns exec N5 ip route add 10.0.10.0/24 via 10.0.50.70 dev V9
sudo ip netns exec N5 ip route add 10.0.20.0/24 via 10.0.50.70 dev V9
sudo ip netns exec N5 ip route add 10.0.30.0/24 via 10.0.50.70 dev V9
sudo ip netns exec N5 ip route add 10.0.60.0/24 via 10.0.50.70 dev V9

# add for N6
sudo ip netns exec N6 ip route add 10.0.10.0/24 via 10.0.60.70 dev V11
sudo ip netns exec N6 ip route add 10.0.20.0/24 via 10.0.60.70 dev V11
sudo ip netns exec N6 ip route add 10.0.30.0/24 via 10.0.60.70 dev V11
sudo ip netns exec N6 ip route add 10.0.40.0/24 via 10.0.60.70 dev V11

# enable IP forwarding at proper network namespaces
sudo ip netns exec N1 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec N2 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec N3 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec N4 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec N5 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec N6 sysctl -w net.ipv4.ip_forward=1

echo "######################################## Begin PartC ########################################"
echo
# Use ​ traceroute​ to show the hops from ​ N1 to N5​ , ​ N3 to N5 ​ , and ​ N3 to N1
echo "######################################## Traceroute N1 to N5 ########################################"
sudo ip netns exec N1 traceroute 10.0.40.70
sleep 3
sudo ip netns exec N1 traceroute 10.0.50.69
sleep 3

echo
echo
echo "######################################## Traceroute N3 to N5 ########################################"
sudo ip netns exec N3 traceroute 10.0.40.70
sleep 3
sudo ip netns exec N3 traceroute 10.0.50.69
sleep 3

echo
echo
echo "######################################## Traceroute N3 to N1 ########################################"
sudo ip netns exec N3 traceroute 10.0.10.69
sleep 3
sudo ip netns exec N3 traceroute 10.0.60.70

# uncomment this to delete all the namespaces created in this script
# delete "${namespaces[@]}"

echo
echo "######################################## End PartC ########################################"