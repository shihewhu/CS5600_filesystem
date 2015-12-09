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
get wang.0 /tmp/wang.0
get wang.1 /tmp/wang.1
get wang.2 /tmp/wang.2
get wang.3 /tmp/wang.3
get wang.4 /tmp/wang.4
get wang.5 /tmp/wang.5
get wang.6 /tmp/wang.6
get wang.7 /tmp/wang.7
get wang.8 /tmp/wang.8
get wang.9 /tmp/wang.9
get wang.10 /tmp/wang.10
get wang.11 /tmp/wang.11
get wang.12 /tmp/wang.12
get wang.13 /tmp/wang.13
get wang.14 /tmp/wang.14
get wang.15 /tmp/wang.15
get wang.16 /tmp/wang.16
get wang.17 /tmp/wang.17
get wang.18 /tmp/wang.18
get wang.19 /tmp/wang.19
get wang.20 /tmp/wang.20
get wang.21 /tmp/wang.21
get wang.22 /tmp/wang.22
get wang.23 /tmp/wang.23
get wang.24 /tmp/wang.24
get wang.25 /tmp/wang.25
get wang.26 /tmp/wang.26
get wang.27 /tmp/wang.27
get wang.28 /tmp/wang.28
ls
ls
ls-l
EOF

