#!/bin/bash
make clean
make
rm foo.img
./mkfs-x6 -size 1M foo.img
./homework -cmdline -image foo.img <<EOF
put wang.3~ wang.0
ls-l
get wang.0 /tmp/wang.3~
EOF
cksum wang.3~ 
cksum /tmp/wang.3~
# ./homework -cmdline -image foo.img
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
