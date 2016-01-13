#!/bin/bash

fail(){
    echo FAILED: $*
    exit 1 
}

# temporary image and directory
#
IMG=/tmp/d.$$.img
DIR=/tmp/d.$$

# unmount on exit, and remove dir and image unless $KEEP is set
#
trap "fusermount -u $DIR; test "$KEEP" || rm -rf $DIR $IMG" 0

# setup
#
mkdir $DIR
./mktest $IMG
./homework -image $IMG $DIR

echo Testing that chmod doesn\'t turn directory into file

# print out commands as we execute them
set -x 

mkdir $DIR/test-dir       || fail mkdir failed
chmod 777 $DIR/test-dir   || fail chmod failed
[ -d $DIR/test-dir ]      || fail $DIR/test-dir isn\'t a directory anymore

set +x
echo SUCCESS