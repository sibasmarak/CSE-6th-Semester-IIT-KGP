"""
Author: Siba Smarak Panigrahi
Roll No.: 18CS10069
Semester: 6
Course: Computer Networks Laboratory (CS39006)
Insitute: Indian Institute of Technology, Kharagpur
"""


################################## Before You Proceed To Run The Python Script ##################################
# Please install geoip2, numpy
# !pip install geoip2
# !pip install numpy

# check that GeoLite2-Country.mmdb and input file (PDML/XML) is in the present working directory
################################## Sure? Everything's Ok? Proceed! ##################################

# Necessary imports
import os
import csv
import time
import argparse
import numpy as np
import geoip2.database
from collections import Counter
import xml.etree.ElementTree as et

def check_via_internet_org(http):
    """
    Check if user accessed the service via the Internet.org proxy or not
    Parameters:
    -----------
    http: http protocol field of a packet

    Returns:
    --------
    a boolean indicating if service was accessed via Internet.org proxy or not
    """

    for field in http:
        field_attributes = field.attrib
        if field_attributes['name'] == 'http.request.line' and field_attributes['showname'] == 'Via: Internet.org\\r\\n':
            return True
    return False

# parser to obtain positional filename argument 
# please ensure the .xml/.pdml file is present in present working directory
parser = argparse.ArgumentParser()
parser.add_argument("filename", help="input the xml-format file", type=str)
args = parser.parse_args()

# if filename argument is given
# create a  filename variable
if args.filename:
	filename = args.filename
# else error and exit
else:
	print('Input a filename! Sorry, I have to exit.')
	exit()

# ensure the current directory as the file_path 
file_path = './'
with open(file_path + filename, 'r') as f:
    # create a string from the file
    # can be passed into ElementTree
    data = ' '.join(f.readlines())

# design the root from the xml data
# use ElementTree
root = et.fromstring(data)

# list to store all the ip-addresses
ip_list = []
for child in root:
    # iterate over all packets
    try:
        for protocol in child:
            # iterate over all protocols of a packet
            if protocol.attrib['name'] == 'http':
                # check for http packet
                http = protocol
                # check if service was used via internet.org proxy
                via_internet_org = check_via_internet_org(http)
                for field in http:
                    # iterate over all fields of http protocol
                    # if service was accessed via internet.org proxy
                    if via_internet_org:
                        field_attributes = field.attrib
                        # check for http.x_forwarded_for field
                        if field_attributes['name'] == 'http.x_forwarded_for':
                            # obtain the user's IP address from 'show' attribute
                            # append to the ip_list
                            ip = field_attributes['show'].strip()
                            ip_list.append(ip)
    except:
        pass

# obtain the list of all unique IP addresses
unique_ip_list = list(set(ip_list))

# use GeoLite2-Country.mmdb to get the country name
with geoip2.database.Reader(file_path + 'GeoLite2-Country.mmdb') as reader:
    country_list = []
    # iterate over the unique_ip_list
    for ip in unique_ip_list:
        try:
            # obtain the country name 
            # append it to country_list
            response = reader.country(ip)
            country_list.append(response.country.name)
        except:
            # if IP Address not in database
            # append country as 'Not Known'
            country_list.append('Not Known')

# use counter to count the country-wise users 
# sort the results
country_counter = Counter(country_list)
country_counter = sorted(country_counter.items(), key=lambda item : (item[1], item[0]), reverse=True)

# write into data.csv
csv_file = './data.csv'
with open(csv_file,'w', newline='') as f:
	writer = csv.writer(f)
	writer.writerows(country_counter)

# print completion and Thank You messages
print("\nDone! The output is written in data.csv file in the present working directory!")
print("\nThank You! Stay Safe! Hope you enjoyed using the script!")