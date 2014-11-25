#!/bin/sh

# This script loads SRMP-LIB from http://pub.ist.ac.at/~vnk/software.html and applies a patch to make SRMP-LIB workable with openGM.
# See README.txt for details.

ZIP_FOLDER=../zip_files/
PATCH_FOLDER=./
SRMP_FILENAME=SRMP-v1.01.zip
SRMP_URL=http://pub.ist.ac.at/~vnk/software/
SRMP_SOURCE_FOLDER=../../SRMP-v1.01.src-patched/
SRMP_PATCH_NAME=SRMP1.01.patch

# check if destination folder already exists
if [ -e "$SRMP_SOURCE_FOLDER" ]
then 
	echo "Source folder already exists, skipping patch."
	exit 0
else
    mkdir $SRMP_SOURCE_FOLDER
fi

# download SRMP
echo "Getting $SRMP_FILENAME from $SRMP_URL ..."
if [ -e "$ZIP_FOLDER$SRMP_FILENAME" ]
then
    echo "$SRMP_FILENAME already exists, skipping download."
else
    wget -q $SRMP_URL$SRMP_FILENAME -P $ZIP_FOLDER
fi

# check if download was successful
if [ -e "$ZIP_FOLDER$SRMP_FILENAME" ]
then :
else
    echo "Couldn't download $SRMP_FILENAME. Check if $SRMP_URL$SRMP_FILENAME is reachable!"
    exit 1
fi

# extract files
echo "Extracting files from $SRMP_FILENAME"
unzip -juqn $ZIP_FOLDER$SRMP_FILENAME -d $SRMP_SOURCE_FOLDER
if [ "$?" = "0" ]
then :
else
    echo "Couldn't extract $SRMP_FILENAME."
    exit 1
fi

# run patch
echo "Patching files..."
patch -s -d $SRMP_SOURCE_FOLDER -p1 < $PATCH_FOLDER$SRMP_PATCH_NAME -N -r -
if [ "$?" = "0" ]
then 
    echo "Patching files done"
else
    echo "Couldn't run patch"
    exit 1
fi
