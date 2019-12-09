#!/bin/bash


display_usage() {
	echo "ERROR: incorrect arguments"
	printf "usage: $0 {toolchain|sim} {tool chain path|host path}\n"
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
	cp libpaho-embed-mqtt3cc.a $2/lib/
	cp MQTTClient-C/src/libpaho-embed-mqtt3cc.so $2/lib/
	
	echo "Copy header files to tool chain"
	mkdir $2/include/mqtt
	cp MQTTClient-C/src/MQTTClient.h $2/include/mqtt
	cp MQTTPacket/src/*.h $2/include/mqtt
	cp MQTTClient-C/src/linux/MQTTLinux.h $2/include/mqtt

	echo "Done"
	exit 0
	
}

simulator_install() {
        echo "Installing MQTT lib to simulator host."
	
        sudo apt install cmake
        cmake CMakeLists.txt
        make -j8 -C MQTTClient-C/src MQTTClient.o
        make -j8 -C MQTTClient-C/src paho-embed-mqtt3cc
        make -j8 -C MQTTClient-C/src linux/MQTTLinux.o
        make -j8 -C MQTTPacket/src/ paho-embed-mqtt3c
        ar -rc libpaho-embed-mqtt3cc.a MQTTClient-C/src/CMakeFiles/paho-embed-mqtt3cc.dir/MQTTClient.c.o MQTTClient-C/src/CMakeFiles/paho-embed-mqtt3cc.dir/linux/MQTTLinux.c.o MQTTPacket/src/CMakeFiles/paho-embed-mqtt3c.dir/*.o
	
	echo "Copy file to host"

	mkdir $2/mqtt
	dest_path=$2/mqtt
	cp libpaho-embed-mqtt3cc.a $dest_path
        cp MQTTClient-C/src/libpaho-embed-mqtt3cc.so $dest_path
   	cp MQTTClient-C/src/MQTTClient.h $dest_path
        cp MQTTPacket/src/*.h $dest_path
        cp MQTTClient-C/src/linux/MQTTLinux.h $dest_path
	
	echo "Done"
        exit 0
}


main() {
	# if less than one arguments supplied, display usage 
	if [  $# -le 1 ]
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
	
	if [ $# -eq 2 ]
	then

		case "$1" in
			toolchain)
				toolchain_install $@
				;;
   			sim)
            		   	simulator_install $@
	         		;;

		        *)
            		   display_usage
		           exit 1
		esac	
	fi
}

main $@
