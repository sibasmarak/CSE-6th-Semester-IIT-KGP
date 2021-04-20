#!/bin/bash

# Name: Siba Smarak Panigrahi
# Roll No.: 18CS10069
# Semester No.: 6
# Assignment No.: 3
# Department of Computer Science and Engineering

############################################ Part B ############################################
# Ensure that H1, H2, H3 and R network namespaces are not available [ check with ip netns list ]
# Also ensure that veth1, veth2, veth3, veth4, veth5, and veth6 are not available [ check with ip link list]
# Ensure sudo mode of operation

# create the R network namespace
sudo ip netns add R





# create the pair of veth interfaces named, veth1 and veth2
sudo ip link add veth1 type veth peer name veth2

# create the H1 network namespace
sudo ip netns add H1

# assign the veth1 interface to the H1 network namespace
sudo ip link set veth1 netns H1
# assign the veth2 interface to the R network namespace
sudo ip link set veth2 netns R

# assign the 10.0.10.69/24 IP address range to the veth1 interface
sudo ip netns exec H1 ip addr add 10.0.10.69/24 dev veth1
# assign the 10.0.10.1/24 IP address range to the veth2 interface
sudo ip netns exec R ip addr add 10.0.10.1/24 dev veth2

# bring up the veth1 and veth2 interface
sudo ip netns exec H1 ip link set veth1 up
sudo ip netns exec R ip link set veth2 up

# bring up the lo interface
sudo ip netns exec H1 ip link set lo up 
sudo ip netns exec R ip link set lo up 

# update the H1 route table by adding a route to 10.0.10.1
sudo ip netns exec H1 ip route add 10.0.10.1 dev veth1
# update the R route table by adding a route to 10.0.10.69
sudo ip netns exec R ip route add 10.0.10.69 dev veth2





# create the pair of veth interfaces named, veth6 and veth3
sudo ip link add veth6 type veth peer name veth3

# create the H2 network namespace
sudo ip netns add H2

# assign the veth6 interface to the H2 network namespace
sudo ip link set veth6 netns H2
# assign the veth3 interface to the R network namespace
sudo ip link set veth3 netns R

# assign the 10.0.20.69/24 IP address range to the veth6 interface
sudo ip netns exec H2 ip addr add 10.0.20.69/24 dev veth6
# assign the 10.0.20.1/24 IP address range to the veth3 interface
sudo ip netns exec R ip addr add 10.0.20.1/24 dev veth3

# bring up the veth6 interface
sudo ip netns exec H2 ip link set veth6 up
# bring up the veth3 interface
sudo ip netns exec R ip link set veth3 up

# bring up the lo interface
sudo ip netns exec H2 ip link set lo up 
sudo ip netns exec R ip link set lo up 

# update the H2 route table by adding a route to 10.0.20.1
sudo ip netns exec H2 ip route add 10.0.20.1 dev veth6

# update the R route table by adding a route to 10.0.20.69
sudo ip netns exec R ip route add 10.0.20.69 dev veth3





# create the pair of veth interfaces named, veth5 and veth4
sudo ip link add veth5 type veth peer name veth4

# create the H3 network namespace
sudo ip netns add H3

# assign the veth5 interface to the H3 network namespace
sudo ip link set veth5 netns H3
# assign the veth4 interface to the R network namespace
sudo ip link set veth4 netns R

# assign the 10.0.30.69/24 IP address range to the veth1 interface
sudo ip netns exec H3 ip addr add 10.0.30.69/24 dev veth5
# assign the 10.0.30.1/24 IP address range to the veth2 interface
sudo ip netns exec R ip addr add 10.0.30.1/24 dev veth4

# bring up the veth5 interface
sudo ip netns exec H3 ip link set veth5 up
# bring up the veth4 interface
sudo ip netns exec R ip link set veth4 up

# bring up the lo interface
sudo ip netns exec H3 ip link set lo up 
sudo ip netns exec R ip link set lo up 

# update the H3 route table by adding a route to 10.0.30.1
sudo ip netns exec H3 ip route add 10.0.30.1 dev veth5

# update the R route table by adding a route to 10.0.30.69
sudo ip netns exec R ip route add 10.0.30.69 dev veth4





# add the default routes
ip netns exec R ip route add default via 10.0.10.1 dev veth2
ip netns exec R ip route add default via 10.0.20.1 dev veth3
ip netns exec R ip route add default via 10.0.30.1 dev veth4
ip netns exec H1 ip route add default via 10.0.10.69 dev veth1
ip netns exec H2 ip route add default via 10.0.20.69 dev veth6
ip netns exec H3 ip route add default via 10.0.30.69 dev veth5





# build brideR
sudo ip netns exec R brctl addbr bridgeR

# add the veth interafaces to brideR
sudo ip netns exec R brctl addif bridgeR veth2
sudo ip netns exec R brctl addif bridgeR veth3
sudo ip netns exec R brctl addif bridgeR veth4

# bring up the bridge bridgeR
sudo ip netns exec R ip link set dev bridgeR up

# enable IP forwarding at R 
sudo ip netns exec R sysctl -w net.ipv4.ip_forward=1
