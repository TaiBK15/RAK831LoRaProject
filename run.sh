#!/bin/bash
echo "welcome"
cd ~/RAK831LoRaProject/Gateway/main
if [ $1 = "make" ]
then
    echo "MAKE AGAIN"
    sudo make clean
    sudo make
fi

case $2 in
     multi_channel)
        echo "RUN MULTI CHANNEL"
        sudo ./rak_multi_channel
	;;
     range_test)
	echo "RUN TEST RANGE"
	sudo ./RAK831_range_test_tx
	;;
     *)
	echo "Nothing chosen"
esac
