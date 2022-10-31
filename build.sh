#!/bin/bash

#-----------------------------#
# Libraries required to build #
#                             #
# suto apt-get install        #
#                             #
#   libx11-dev libgl-dev      #
#   libssl-dev                #
#                             #
#-----------------------------#

#Setup Compiler Options Here

CPP=g++
GPP=g++
OUTPUT="test.elf"

    #Debug Mode enables certain warnings, debug symbols, and address sanitization to help mediate memory address issues;
    # turn off for best performance!
DEBUGMODE=0
LINK_ONLY=0

VERBOSE=0
ASYNC_BUILD=1
REBUILD_APPLICATION=1

SOURCE_DIRECTORY="src"
COMPILER_FLAGS=""
ADDITIONAL_LIBRARIES="-static-libstdc++ -lX11 -lGL -lpthread -lstdc++fs -lcrypto"
ADDITIONAL_LIBDIRS=""
ADDITIONAL_INCLUDEDIRS="-Iinclude -Ilibraries/librapidjson/include"
LINKER_FLAGS=""

OBJ_DIR=".objs64"


#Compilers older than GCC 10.2.0 need to use -std=c++2a
# -- Automatically determine G++ version and use correct flag --

echo "--------------------------------------------------"
echo "-       Initializing MME Build Toolchain         -"
echo "--------------------------------------------------"

CXX2A="-std=c++2a"
CXX20="-std=c++20"

if ! command -v $GPP &> /dev/null; then
    echo "Could not find ${GPP}; You must install ${GPP} first using: apt install ${GPP}"
    exit
fi

VERSION=$( $GPP -dumpspecs | grep *version: -s -A 1 | grep -v *version )
_VERSION=()
_IFS=$IFS;
IFS='.';
for vz in $VERSION; do
    _VERSION+=( $vz )
done
IFS=$_IFS # restore previous IFS

HIGHER_VERSION=0
if [ ${_VERSION[0]} -eq 10 ]; then
    if [ ${_VERSION[1]} -eq 2 ]; then
        if [ ${_VERSION[2]} -ge 0 ]; then
            HIGHER_VERSION=1
        fi
    elif [ ${_VERSION[1]} -ge 2 ]; then
        HIGHER_VERSION=1
    else
        HIGHER_VERSION=0
    fi
elif [ ${_VERSION[0]} -ge 10 ]; then
    HIGHER_VERSION=1
else
    HIGHER_VERSION=0
fi

if [ $HIGHER_VERSION -eq 1 ]; then
    COMPILER_FLAGS+=" ${CXX20}"
else
    COMPILER_FLAGS+=" ${CXX2A}"
fi

echo "|     Using ${GPP} version ${VERSION}     |"
echo "========================================="

if [ -f "$OUTPUT" ]; then
    rm $OUTPUT
fi

if [ $LINK_ONLY -eq 0 ]; then

    if [ $DEBUGMODE -eq 1 ]; then
        DEBUG_INFO="-ggdb -g -fsanitize=address -Wshadow-compatible-local"
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
        objfile="$(basename "$filename" .cpp).o"

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

sleep 5 #sleep for 5 seconds to prevent any linkage issues

files=""
for filename in "$OBJ_DIR"/*.o; do
    files="$files $filename"
done

if [ $DEBUGMODE -eq 1 ]; then
    LINKER_FLAGS+=" -fsanitize=address"
fi

if [ $VERBOSE -eq 1 ]; then
    echo "$GPP $LINKER_FLAGS $ADDITIONAL_LIBDIRS -o $OUTPUT $files $ADDITIONAL_LIBRARIES"
fi

$GPP $LINKER_FLAGS $ADDITIONAL_LIBDIRS -o $OUTPUT $files $ADDITIONAL_LIBRARIES

if [ -f "$OUTPUT" ]; then
    echo "---------------------------"
    echo "-  MME Build Completed!   -"
    echo "---------------------------"
else
    echo "-------------------------"
    echo "-   MME Build FAILED!   -"
    echo "-  See Log For Details  -"
    echo "-------------------------"
fi