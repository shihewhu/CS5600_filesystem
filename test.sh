#!/bin/bash
rm foo.img
./mktest foo.img
sleep 0.1
make clean
make
./homework -cmdline -image foo.img <<EOF
rm file.7
rm file.A
rm dir1/file.0
rm dir1/file.2
rm dir1/file.270
rmdir dir1
put wang.3
ls
EOF
# cp ./foo.img /tmp/foo1.img
# ./homework -cmdline -image foo.img <<EOF
# ls
# ls
# EOF
# cp ./foo.img /tmp/foo2.img
# diff /tmp/foo1.img /tmp/foo2.img
