#!/bin/bash

BUILD_DIRECTORY="build";
BIN_NAME="MidiPlayer"

# Generate Cmake file
if [ ! -d $BUILD_DIRECTORY ]
then
	mkdir $BUILD_DIRECTORY;
	cmake -S . -B $BUILD_DIRECTORY -DCMAKE_BUILD_TYPE=Debug
	# cmake -S . -B $BUILD_DIRECTORY -DCMAKE_BUILD_TYPE=Release
fi

# Create application preset directory
if [ ! -d $PRESET_DIRECTORY ]
then
	mkdir $PRESET_DIRECTORY;
fi

# Compile and launch application
make -C $BUILD_DIRECTORY;
if [ "$?" == 0 ]
then
	./$BUILD_DIRECTORY/$BIN_NAME
else
	printf "\033[31m-------------------- COMPILATION FAILED --------------------\033[0m\n";
	exit 1;
fi
