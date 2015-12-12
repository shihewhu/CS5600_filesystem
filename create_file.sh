#!/usr/bin/env bash

#helper function
i=1
testing_file="put-test-file.$i"
output_path="/tmp/$file-to-be-tested"
cksum=$(cksum testing_file)
./homework -cmdline -image foo.img << EOF
put $testing_file
get $testing_file $output_path
quit
EOF

