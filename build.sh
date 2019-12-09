#!/bin/bash


display_usage() {
	echo "ERROR: incorrect arguments"
	printf "usage: $0 <tool chain path>\n"
	printf "Note: path to tool chain should look like:\n	<toolchain_dir_path/toolchain_name/sysroot/usr>\n"
}

toolchain_install() {
	echo "Installing MQTT lib to toll chain."

	sudo apt install cmake
	cmake CMakeLists.txt
	make -j8 -C MQTTClient-C/src MQTTClient.o
	make -j8 -C MQTTClient-C/src paho-embed-mqtt3cc
	make -j8 -C MQTTClient-C/src linux/MQTTLinux.o
	make -j8 -C MQTTPacket/src/ paho-embed-mqtt3c
	ar -rc libpaho-embed-mqtt3cc.a MQTTClient-C/src/CMakeFiles/paho-embed-mqtt3cc.dir/MQTTClient.c.o MQTTClient-C/src/CMakeFiles/paho-embed-mqtt3cc.dir/linux/MQTTLinux.c.o MQTTPacket/src/CMakeFiles/paho-embed-mqtt3c.dir/*.o

	echo "Copy library file to tool chain"
	cp libpaho-embed-mqtt3cc.a $1/lib/
	cp MQTTClient-C/src/libpaho-embed-mqtt3cc.so $1/lib/
	
	echo "Copy header files to tool chain"
	mkdir $1/include/mqtt
	cp MQTTClient-C/src/MQTTClient.h $1/include/mqtt
	cp MQTTPacket/src/*.h $1/include/mqtt
	cp MQTTClient-C/src/linux/MQTTLinux.h $1/include/mqtt

	echo "Done"
	exit 0
	
}


main() {
	# if less than one arguments supplied, display usage 
	if [  $# -eq 0	]
	then 
		display_usage
		exit 1
	fi 
 
	# check whether user had supplied -h or --help . If yes display usage 
	if [[ ( $# == "--help") ||  $# == "-h" ]] 
	then 
		display_usage
		exit 0
	fi 
 
	# display usage if the script is not run as root user 
	if [[ $USER != "root" ]]; then 
		echo "This script must be run as root!" 
		exit 1
	fi

	toolchain_install $@
}

main $@
