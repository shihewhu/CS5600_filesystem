#!/bin/bash
rm foo.img
./mktest foo.img
sleep 0.1
make clean
make
./homework -cmdline -image foo.img <<EOF
put wang.1 shi.0
put wang.1 shi.1
put wang.1 shi.2
put wang.1 shi.3
put wang.1 shi.4
put wang.1 shi.5
put wang.1 shi.6
put wang.1 shi.7
put wang.1 shi.8
put wang.1 shi.9
put wang.1 shi.10
put wang.1 shi.11
put wang.1 shi.12
put wang.1 shi.13
put wang.1 shi.14
put wang.1 shi.15
put wang.1 shi.16
put wang.1 shi.17
put wang.1 shi.18
put wang.1 shi.19
put wang.1 shi.20
put wang.1 shi.21
put wang.1 shi.22
put wang.1 shi.23
put wang.1 shi.24
put wang.1 shi.25
put wang.1 shi.26
put wang.1 shi.27
put wang.1 shi.28
put wang.1 shi.29
ls
EOF
# cp ./foo.img /tmp/foo1.img
# ./homework -cmdline -image foo.img <<EOF
# ls
# ls
# EOF
# cp ./foo.img /tmp/foo2.img
# diff /tmp/foo1.img /tmp/foo2.img
