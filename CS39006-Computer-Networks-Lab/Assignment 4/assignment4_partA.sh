#!/bin/bash

# declare array of IP Addresses
# declare array of namespaces
declare -a ips=("10.0.10.69" "10.0.10.70" "10.0.20.69" "10.0.20.70" "10.0.30.69" "10.0.30.70")
declare -a namespaces=("N1" "N2" "N3" "N4")

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

# assign the veth interfaces to the respective network namespaces
# assign as per the problem statement
sudo ip link set V1 netns N1
sudo ip link set V2 netns N2
sudo ip link set V3 netns N2
sudo ip link set V4 netns N3
sudo ip link set V5 netns N3
sudo ip link set V6 netns N4

# assign the corresponding IP address range to the veth interfaces
# assign as per the problem statement
sudo ip netns exec N1 ip addr add 10.0.10.69/24 dev V1
sudo ip netns exec N2 ip addr add 10.0.10.70/24 dev V2
sudo ip netns exec N2 ip addr add 10.0.20.69/24 dev V3
sudo ip netns exec N3 ip addr add 10.0.20.70/24 dev V4
sudo ip netns exec N3 ip addr add 10.0.30.69/24 dev V5
sudo ip netns exec N4 ip addr add 10.0.30.70/24 dev V6

# bring up the veth interfaces
sudo ip netns exec N1 ip link set V1 up
sudo ip netns exec N2 ip link set V2 up
sudo ip netns exec N2 ip link set V3 up
sudo ip netns exec N3 ip link set V4 up
sudo ip netns exec N3 ip link set V5 up
sudo ip netns exec N4 ip link set V6 up

# bring up the lo interfaces for each network namespace
# sanity check
sudo ip netns exec N1 ip link set lo up
sudo ip netns exec N2 ip link set lo up
sudo ip netns exec N3 ip link set lo up
sudo ip netns exec N4 ip link set lo up

# populate the routing tables for each namespace
# go through each namespace and find the routes to be added
# ensure there is a return route also to each added routes
sudo ip netns exec N1 ip route add 10.0.20.0/24 via 10.0.10.70 dev V1
sudo ip netns exec N1 ip route add 10.0.30.0/24 via 10.0.10.70 dev V1
sudo ip netns exec N2 ip route add 10.0.30.0/24 via 10.0.20.70 dev V3
sudo ip netns exec N3 ip route add 10.0.10.0/24 via 10.0.20.69 dev V4
sudo ip netns exec N4 ip route add 10.0.10.0/24 via 10.0.30.69 dev V6
sudo ip netns exec N4 ip route add 10.0.20.0/24 via 10.0.30.69 dev V6

# enable IP forwarding at proper network namespaces
sudo ip netns exec N2 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec N3 sysctl -w net.ipv4.ip_forward=1

# ping the interfaces
echo "######################################## Begin PartA ########################################"
echo

ping "N1" "${ips[@]}"
ping "N2" "${ips[@]}"
ping "N3" "${ips[@]}"
ping "N4" "${ips[@]}"

# uncomment this to delete all the namespaces created in this script
# delete "${namespaces[@]}"

echo
echo "######################################## End PartA ########################################"