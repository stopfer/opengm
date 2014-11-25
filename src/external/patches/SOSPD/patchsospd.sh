#!/bin/sh

# This script loads SOSPD-LIB from https://github.com/letterx/sospd and applies a patch to make SOSPD-LIB workable with openGM.
# See README.txt for details.

ZIP_FOLDER=../zip_files/
PATCH_FOLDER=./
SOSPD_FILENAME=master.zip
SOSPD_LOCAL_FILENAME=SOSPD.zip
SOSPD_TOP_LAYER_FOLDER=sospd-master
SOSPD_URL=https://github.com/letterx/sospd/archive/
SOSPD_SOURCE_FOLDER=../../SOSPD.src-patched/
SOSPD_PATCH_NAME=SOSPD.patch

# check if destination folder already exists
if [ -e "$SOSPD_SOURCE_FOLDER" ]
then 
	echo "Source folder already exists, skipping patch."
	exit 0
else
    mkdir $SOSPD_SOURCE_FOLDER
fi

# download SOSPD
echo "Getting $SOSPD_FILENAME from $SOSPD_URL and storing it as $SOSPD_LOCAL_FILENAME ..."
if [ -e "$ZIP_FOLDER$SOSPD_LOCAL_FILENAME" ]
then
    echo "$SOSPD_LOCAL_FILENAME already exists, skipping download."
else
    wget -q $SOSPD_URL$SOSPD_FILENAME -O $ZIP_FOLDER$SOSPD_LOCAL_FILENAME
fi

# check if download was successful
if [ -e "$ZIP_FOLDER$SOSPD_LOCAL_FILENAME" ]
then :
else
    echo "Couldn't download $SOSPD_FILENAME. Check if $SOSPD_URL$SOSPD_FILENAME is reachable!"
    exit 1
fi

# extract files
echo "Extracting files from $SOSPD_LOCAL_FILENAME"
unzip -uqn $ZIP_FOLDER$SOSPD_LOCAL_FILENAME -d $SOSPD_SOURCE_FOLDER
if [ "$?" = "0" ]
then :
else
    echo "Couldn't extract $SOSPD_LOCAL_FILENAME."
    exit 1
fi

# remove container folder
echo "Moving files"
mv $SOSPD_SOURCE_FOLDER/sospd-master/* $SOSPD_SOURCE_FOLDER
if [ "$?" = "0" ]
then :
else
    echo "Couldn't move files."
    exit 1
fi
rm -r $SOSPD_SOURCE_FOLDER/sospd-master
if [ "$?" = "0" ]
then :
else
    echo "Couldn't remove folder."
    exit 1
fi

# run patch
#echo "Patching files..."
#patch -s -d $SOSPD_SOURCE_FOLDER -p1 < $PATCH_FOLDER$SOSPD_PATCH_NAME -N -r -
#if [ "$?" = "0" ]
#then 
#    echo "Patching files done"
#else
#    echo "Couldn't run patch"
#    exit 1
#fi
