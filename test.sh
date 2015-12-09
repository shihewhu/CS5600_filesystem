#!/bin/bash
rm foo.img
./mktest foo.img
sleep 0.1
make clean
make
./homework -cmdline -image foo.img <<EOF
put wang.1 wang.0
put wang.1 wang.1
put wang.1 wang.2
put wang.1 wang.3
put wang.1 wang.4
put wang.1 wang.5
put wang.1 wang.6
put wang.1 wang.7
put wang.1 wang.8
put wang.1 wang.9
put wang.1 wang.10
put wang.1 wang.11
put wang.1 wang.12
put wang.1 wang.13
put wang.1 wang.14
put wang.1 wang.15
put wang.1 wang.16
put wang.1 wang.17
put wang.1 wang.18
put wang.1 wang.19
put wang.1 wang.20
put wang.1 wang.21
put wang.1 wang.22
put wang.1 wang.23
put wang.1 wang.24
put wang.1 wang.25
put wang.1 wang.26
put wang.1 wang.27
put wang.1 wang.28
put wang.1 wang.29
ls
EOF
# cp ./foo.img /tmp/foo1.img
# ./homework -cmdline -image foo.img <<EOF
# ls
# ls
# EOF
# cp ./foo.img /tmp/foo2.img
# diff /tmp/foo1.img /tmp/foo2.img
