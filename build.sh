#!/bin/bash

#-----------------------------#
# Libraries required to build #
#                             #
# suto apt-get install        #
#                             #
#   libx11-dev libgl-dev      #
#   libpng-dev libssl-dev     #
#                             #
#-----------------------------#

#Setup Compiler Options Here

CPP=g++
GPP=g++
OUTPUT="MME.elf"

DEBUGMODE=0
LINK_ONLY=0

VERBOSE=0
ASYNC_BUILD=1
REBUILD_APPLICATION=1

SOURCE_DIRECTORY="src"
    #Compilers older than GCC 10.2.0 need to use -std=c++2a
COMPILER_FLAGS="-std=c++2a"
ADDITIONAL_LIBRARIES="-static-libstdc++ -lX11 -lGL -lpthread -lpng -lstdc++fs -lcrypto"
ADDITIONAL_LIBDIRS=""
ADDITIONAL_INCLUDEDIRS="-Iinclude -Ilibraries/librapidjson/include"

OBJ_DIR=".objs64"

if [ -f "$OUTPUT" ]; then
    rm $OUTPUT
fi

if [ $LINK_ONLY -eq 0 ]; then

    if [ $DEBUGMODE -eq 1 ]; then
        DEBUG_INFO="-ggdb -g"
    else
        DEBUG_INFO="-s"
    fi

	if [ $REBUILD_APPLICATION -eq 1 ]; then
		rm $OBJ_DIR/*.o
	fi

    if [ ! -d "$OBJ_DIR/" ]; then
        echo "Creating Object Directory Structure..."
        mkdir "$OBJ_DIR/"
    fi


    echo "Building API Files..."
    procs=[]
    n=0
    cppfiles=$(find $SOURCE_DIRECTORY -type f -name "*.cpp")

    for filename in $cppfiles; do
        objfile="$(basename "$filename" .cpp)$n.o"
        
        #echo "Iterate: $filename"
        #echo "objfile=$objfile"

        if [ ! -f $OBJ_DIR/$objfile ]; then
            echo "Building $objfile"

            if [ $ASYNC_BUILD -eq 1 ]; then
                $CPP $ADDITIONAL_INCLUDEDIRS $COMPILER_FLAGS $DEBUG_INFO -c $filename -o $OBJ_DIR/$objfile &
                procs[${n-1}]=$!
            else
                $CPP $ADDITIONAL_INCLUDEDIRS $COMPILER_FLAGS $DEBUG_INFO -c $filename -o $OBJ_DIR/$objfile
            fi

            if [ $VERBOSE -eq 1 ]; then
                echo "$CPP $ADDITIONAL_INCLUDEDIRS $COMPILER_FLAGS $DEBUG_INFO -c $filename -o $OBJ_DIR/$objfile"
            fi
        fi
        n=$((n + 1))
    done

    for pid in ${procs[*]}; do
        wait $pid
    done

fi

# Linking

echo "Linking Executable..."

files=""
for filename in "$OBJ_DIR"/*.o; do
    files="$files $filename"
done

if [ $VERBOSE -eq 1 ]; then
    echo "$GPP $ADDITIONAL_LIBDIRS -o $OUTPUT $files $ADDITIONAL_LIBRARIES"
fi

$GPP $ADDITIONAL_LIBDIRS -o $OUTPUT $files $ADDITIONAL_LIBRARIES

if [ -f "$OUTPUT" ]; then
    echo "Build Complete"
else
    echo "Build Failed"
fi