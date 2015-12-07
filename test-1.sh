#!/usr/bin/env bash

make
./homework -cmdline -image foo.img << EOF > /tmp/test1-output
get file.A /tmp/test1-file.A
quit
EOF

cat /tmp/test1-file.A
echo ""
cat /tmp/test1-output