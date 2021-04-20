#!/bin/bash

# declare array of IP Addresses
# declare array of namespaces
declare -a ips=("10.0.10.69" "10.0.10.70" "10.0.20.69" "10.0.20.70" "10.0.30.69" "10.0.30.70" "10.0.40.69" "10.0.40.70" "10.0.50.69" "10.0.50.70" "10.0.60.69" "10.0.60.70")
declare -a namespaces=("H1" "H2" "H3" "H4" "R1" "R2" "R3")

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
sudo ip link set V1 netns H1
sudo ip link set V2 netns R1
sudo ip link set V3 netns H2
sudo ip link set V4 netns R1
sudo ip link set V5 netns R1
sudo ip link set V6 netns R2
sudo ip link set V7 netns R2
sudo ip link set V8 netns R3
sudo ip link set V9 netns R3
sudo ip link set V10 netns H3
sudo ip link set V11 netns R3
sudo ip link set V12 netns H4

# assign the corresponding IP address range to the veth interfaces
# assign as per the problem statement
sudo ip netns exec H1 ip addr add 10.0.10.69/24 dev V1
sudo ip netns exec R1 ip addr add 10.0.10.70/24 dev V2
sudo ip netns exec H2 ip addr add 10.0.20.69/24 dev V3
sudo ip netns exec R1 ip addr add 10.0.20.70/24 dev V4
sudo ip netns exec R1 ip addr add 10.0.30.69/24 dev V5
sudo ip netns exec R2 ip addr add 10.0.30.70/24 dev V6
sudo ip netns exec R2 ip addr add 10.0.40.69/24 dev V7
sudo ip netns exec R3 ip addr add 10.0.40.70/24 dev V8
sudo ip netns exec R3 ip addr add 10.0.50.69/24 dev V9
sudo ip netns exec H3 ip addr add 10.0.50.70/24 dev V10
sudo ip netns exec R3 ip addr add 10.0.60.69/24 dev V11
sudo ip netns exec H4 ip addr add 10.0.60.70/24 dev V12

# bring up the veth interfaces
sudo ip netns exec H1 ip link set V1 up
sudo ip netns exec R1 ip link set V2 up
sudo ip netns exec H2 ip link set V3 up
sudo ip netns exec R1 ip link set V4 up
sudo ip netns exec R1 ip link set V5 up
sudo ip netns exec R2 ip link set V6 up
sudo ip netns exec R2 ip link set V7 up
sudo ip netns exec R3 ip link set V8 up
sudo ip netns exec R3 ip link set V9 up
sudo ip netns exec H3 ip link set V10 up
sudo ip netns exec R3 ip link set V11 up
sudo ip netns exec H4 ip link set V12 up

# bring up the lo interfaces for each network namespace
# sanity check
sudo ip netns exec H1 ip link set lo up
sudo ip netns exec R1 ip link set lo up
sudo ip netns exec H2 ip link set lo up
sudo ip netns exec R2 ip link set lo up
sudo ip netns exec R3 ip link set lo up
sudo ip netns exec H3 ip link set lo up
sudo ip netns exec H4 ip link set lo up

# populate the routing tables for each namespace
# go through each namespace and find the routes to be added
# ensure there is a return route also to each added routes

# add for H1
sudo ip netns exec H1 ip route add 10.0.20.0/24 via 10.0.10.70 dev V1
sudo ip netns exec H1 ip route add 10.0.30.0/24 via 10.0.10.70 dev V1
sudo ip netns exec H1 ip route add 10.0.40.0/24 via 10.0.10.70 dev V1
sudo ip netns exec H1 ip route add 10.0.50.0/24 via 10.0.10.70 dev V1
sudo ip netns exec H1 ip route add 10.0.60.0/24 via 10.0.10.70 dev V1

# add for H2
sudo ip netns exec H2 ip route add 10.0.10.0/24 via 10.0.20.70 dev V3
sudo ip netns exec H2 ip route add 10.0.30.0/24 via 10.0.20.70 dev V3
sudo ip netns exec H2 ip route add 10.0.40.0/24 via 10.0.20.70 dev V3
sudo ip netns exec H2 ip route add 10.0.50.0/24 via 10.0.20.70 dev V3
sudo ip netns exec H2 ip route add 10.0.60.0/24 via 10.0.20.70 dev V3

# add for H3
sudo ip netns exec H3 ip route add 10.0.10.0/24 via 10.0.50.69 dev V10
sudo ip netns exec H3 ip route add 10.0.20.0/24 via 10.0.50.69 dev V10
sudo ip netns exec H3 ip route add 10.0.30.0/24 via 10.0.50.69 dev V10
sudo ip netns exec H3 ip route add 10.0.40.0/24 via 10.0.50.69 dev V10
sudo ip netns exec H3 ip route add 10.0.60.0/24 via 10.0.50.69 dev V10

# add for H4
sudo ip netns exec H4 ip route add 10.0.10.0/24 via 10.0.60.69 dev V12
sudo ip netns exec H4 ip route add 10.0.20.0/24 via 10.0.60.69 dev V12
sudo ip netns exec H4 ip route add 10.0.30.0/24 via 10.0.60.69 dev V12
sudo ip netns exec H4 ip route add 10.0.40.0/24 via 10.0.60.69 dev V12
sudo ip netns exec H4 ip route add 10.0.50.0/24 via 10.0.60.69 dev V12

# add for R1
sudo ip netns exec R1 ip route add 10.0.40.0/24 via 10.0.30.70 dev V5
sudo ip netns exec R1 ip route add 10.0.50.0/24 via 10.0.30.70 dev V5
sudo ip netns exec R1 ip route add 10.0.60.0/24 via 10.0.30.70 dev V5

# add for R2
sudo ip netns exec R2 ip route add 10.0.10.0/24 via 10.0.30.69 dev V6
sudo ip netns exec R2 ip route add 10.0.20.0/24 via 10.0.30.69 dev V6
sudo ip netns exec R2 ip route add 10.0.50.0/24 via 10.0.40.70 dev V7
sudo ip netns exec R2 ip route add 10.0.60.0/24 via 10.0.40.70 dev V7

# add for R3
sudo ip netns exec R3 ip route add 10.0.10.0/24 via 10.0.40.69 dev V8
sudo ip netns exec R3 ip route add 10.0.20.0/24 via 10.0.40.69 dev V8
sudo ip netns exec R3 ip route add 10.0.30.0/24 via 10.0.40.69 dev V8

# enable IP forwarding at proper network namespaces
sudo ip netns exec R1 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec R2 sysctl -w net.ipv4.ip_forward=1
sudo ip netns exec R3 sysctl -w net.ipv4.ip_forward=1

# ping the interfaces
# Use traceroute to show the hops from ​ H1 to H4​ , ​ H3 to H4 ​ , and ​ H4 to H2
echo "######################################## Begin PartA ########################################"
echo

ping "H1" "${ips[@]}"
ping "H2" "${ips[@]}"
ping "H3" "${ips[@]}"
ping "H4" "${ips[@]}"
ping "R1" "${ips[@]}"
ping "R2" "${ips[@]}"
ping "R3" "${ips[@]}"


echo "######################################## Traceroute H1 to H4 ########################################"
sudo ip netns exec H1 traceroute 10.0.60.70
sleep 3

echo "######################################## Traceroute H3 to H4 ########################################"
sudo ip netns exec H3 traceroute 10.0.60.70
sleep 3

echo "######################################## Traceroute H4 to H2 ########################################"
sudo ip netns exec H4 traceroute 10.0.20.69
sleep 3

# uncomment this to delete all the namespaces created in this script
# delete "${namespaces[@]}"

echo
echo "######################################## End PartB ########################################"