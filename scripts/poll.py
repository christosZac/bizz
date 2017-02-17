#!/usr/bin/env python
import socket
import sys
import requests
import time

from coapthon.client.helperclient import HelperClient
from coapthon.utils import parse_uri

__author__ = 'Christos'

client = None
# Polling interval in seconds
TIME_INTERVAL = 60

def main():  # pragma: no cover
    global client    
    # Define a dictionary where the settings file will be read into.
    dict = {}
    with open("settings") as f:
        # For every line in the file
        for line in f:
            # Remove special characters & whitespaces.
            line = line.strip('\t\n\r')
            line = line.replace(" ", "")
            # Split on the '=' and store in the dictionary.
            (key, val) = line.split("=")
            dict[key] = val

    # Define the CoAP operation (GET).
    op = dict["OPERATION"]
    # Define the device and resource's paths.
    path = dict["PATH"]
    temp_path = dict["TEMP_PATH"]
    hum_path = dict["HUM_PATH"]
    load_path = dict["LOAD_PATH"]
    # Parse the cloud credentials.
    relayrDevice = dict["DEVICE_ID"]
    relayrToken = dict["USER_TOKEN"]
    # Setting the headers and the respective URL.
    relayrUrl = "https://api.relayr.io/devices/" + relayrDevice + "/data"
    relayrHeaders = {'authorization' : 'Bearer ' + relayrToken , 'cache-control' : 'no-cache' , 'content-type' : 'application/json'}
    # Variables to store sensor data.
    dashboardHum = None
    dashboardTemp = None
    dashboardLoad = None

    # A little path check.
    if not path.startswith("coap://"):
        print "Path must be conform to coap://host[:port]/path"
        sys.exit(2)
    # Parse the path.
    host, port, path = parse_uri(path)
    try:
        tmp = socket.gethostbyname(host)
        host = tmp
    except socket.gaierror:
        pass
    
    if path is None:
        print "Path cannot be empty for a GET request"
        sys.exit(2)

    while True:
        try:
            # Set timer. Data will be sent when the timer expires.
            time.sleep(TIME_INTERVAL)
            # Start making the requests to different resources.
            # The returned values are stored in different variables.
            client = HelperClient(server=(host, port))
            response_temp = client.get(path + temp_path) 
            print response_temp.pretty_print()
            dashboardTemp = {'meaning' : 'temperature' , 'value' : float(response_temp.payload) /10 }
            client.stop()

            client = HelperClient(server=(host, port))
            response_hum = client.get(path + hum_path)
            print response_hum.pretty_print()
            dashboardHum = {'meaning' : 'humidity' , 'value' : float(response_hum.payload) / 10 }
            client.stop()

            client = HelperClient(server=(host, port))
            response_load = client.get(path + load_path)
            print response_load.pretty_print()
            dashboardLoad = {'meaning' : 'load' , 'value' : float(response_load.payload) /1000 }
            client.stop()
        except Exceptions as e:
            print type(e)

        print dashboardTemp 
        print dashboardHum
        print dashboardLoad

        # For every resource, upload the data to the cloud through the given APIs.
        # Debug printing the status of the request.
        try:
            r = requests.post(relayrUrl , headers=relayrHeaders, json=dashboardTemp)
            #print r.status_code
            r = requests.post(relayrUrl , headers=relayrHeaders, json=dashboardHum)
            #print r.status_code
            r = requests.post(relayrUrl , headers=relayrHeaders, json=dashboardLoad)
            #print r.status_code
        except Exception as e:
            print type(e)

        response_temp = None
        response_hum = None
        response_load = None

    #client.stop()

if __name__ == '__main__':  # pragma: no cover
    main()
