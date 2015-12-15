#!/bin/bash
make clean
make
rm foo.img
./mkfs-x6 -size 2M foo.img
./homework -cmdline -image foo.img <<EOF
put wang.3~ wang.0
rm wang.0
put wang.3~ wang.0
ls-l
rm wang.0
put wang.3~ wang.0
ls-l
rm wang.0
put wang.3~ wang.0
ls-l
EOF
./homework -cmdline -image foo.img
# cp ./foo.img /tmp/foo1.img
# ./homework -cmdline -image foo.img <<EOF
# ls
# ls
# EOF
# cp ./foo.img /tmp/foo2.img
# diff /tmp/foo1.img /tmp/foo2.img
# put wang.3~ wang.1
# put wang.3~ wang.2
# put wang.3~ wang.3
# put wang.3~ wang.4
