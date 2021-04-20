#!/bin/bash

# Name: Siba Smarak Panigrahi
# Roll No.: 18CS10069
# Semester No.: 6
# Assignment No.: 3
# Department of Computer Science and Engineering

############################################ Part A ############################################

# Ensure that siba0 and siba1 namespaces are not available [ check with ip netns list ]
# Also ensure that veth1 and veth2 are not available [ check with ip link list]
# Ensure sudo mode of operation

# create the pair of veth interfaces named, veth0 and veth1
sudo ip link add veth0 type veth peer name veth1

# create the siba0 and siba1 network namespace
sudo ip netns add siba0
sudo ip netns add siba1

# assign the veth0, veth1 interface to the siba0, siba1 rerspectively network namespace
sudo ip link set veth0 netns siba0
sudo ip link set veth1 netns siba1

# assign the 10.1.1.0/24 IP address range to the veth0 interface
sudo ip netns exec siba0 ip addr add 10.1.1.0/24 dev veth0

# assign the 10.0.2.0/24 IP address range to the veth1 interface
sudo ip netns exec siba1 ip addr add 10.1.2.0/24 dev veth1

# bring up the veth0 and veth1 interface
sudo ip netns exec siba0 ip link set veth0 up
sudo ip netns exec siba1 ip link set veth1 up

# bring up the lo interface
sudo ip netns exec siba0 ip link set lo up 
sudo ip netns exec siba1 ip link set lo up 

# update the siba0 route table by adding a route to 10.0.2.0
sudo ip netns exec siba0 ip route add 10.1.2.0 dev veth0

# update the siba1 route table by adding a route to 10.0.1.0
sudo ip netns exec siba1 ip route add 10.1.1.0 dev veth1
