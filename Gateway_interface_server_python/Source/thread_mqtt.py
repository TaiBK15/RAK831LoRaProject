import socket
import thread
import threading
import time
import sys
import json

try:
    import paho.mqtt.client as mqtt
except ImportError:

    import os
    import inspect
    cmd_subfolder = os.path.realpath(
        os.path.abspath(os.path.join(os.path.split(inspect.getfile(inspect.currentframe()))[0], "../src")))
    if cmd_subfolder not in sys.path:
        sys.path.insert(0, cmd_subfolder)

    import paho.mqtt.client as mqtt
    print "import error"


# Define event callbacks

def on_connect(mosq, obj, rc):
    """
    Connect MQTT Server
    :param mosq:
    :param obj:
    :param rc:
    :return:
    """
    print ("on_connect:: Connected with result code " + str(rc))
    print("rc: " + str(rc))


def on_message(mosq, obj, msg):
    """
    Message from MQTT Server
    :param mosq:
    :param obj:
    :param msg:
    :return:
    """
    print ("on_message:: this means  I got a message from broker for this topic")
    print(msg.topic + " " + str(msg.qos) + " " + str(msg.payload))
    # Send payload to C process
    conn.sendall(msg.payload)


def on_publish(mosq, obj, mid):
    """
    Show message ID when publishing to MQTT Server
    :param mosq:
    :param obj:
    :param mid:
    :return:
    """
    print("publish to cloudmqtt " + str(mid))


def on_subscribe(mosq, obj, mid, granted_qos):
    """

    :param mosq:
    :param obj:
    :param mid:
    :param granted_qos:
    :return:
    """
    print("This means broker has acknowledged my subscribe request")
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def on_log(mosq, obj, level, string):
    """

    :param mosq:
    :param obj:
    :param level:
    :param string:
    :return:
    """
    print(string)


client = mqtt.Client()
# Assign event callbacks
client.on_message = on_message
client.on_connect = on_connect
client.on_publish = on_publish
client.on_subscribe = on_subscribe

# Set up password and IP
client.username_pw_set("asxdszmm", "NJw3kQLh_Mze")
client.connect('m10.cloudmqtt.com', 10452, 60)


# Define functions for threads
# ------Thread 1: Publish = Send message to Cloud------
def wait_and_publish_to_mqtt(mess):
    """
    Thread 1: Wait until receive packets and publish packets to Server
    :param mess:
    :return:
    """
    global conn
    global addr
    HOST = 'localhost'
    PORT = 8080
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen(1)
    conn, addr = s.accept()
    print('Connected by', addr)
    while True:
        # data from C gateway
        data = conn.recv(1024)
        threadLock.acquire()
        client.publish("UPLINK", str(data))
        threadLock.release()


# Create two threads as follows
try:
    thread.start_new_thread(wait_and_publish_to_mqtt, ("create thread sub\n",))
except:
    print "Error: unable to start thread"
# Synchronizing Thread
threadLock = threading.Lock()
# Main thread: Subcribe = Receive message Control from Cloud
client.subscribe("DOWNLINK", 0)
client.loop_forever()
