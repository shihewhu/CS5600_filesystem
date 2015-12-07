#!/usr/bin/env bash

make
if [ -f /tmp/test1-output ]
	then 
		rm /tmp/test1-output
		echo "remove test1-output"
fi
if [ -f /tmp/test1-$1 ]
	then 
		rm /tmp/test1-$1
		echo "remove file"
fi
./homework -cmdline -image foo.img << EOF > /tmp/test1-output
get $1 /tmp/test1-$1
quit
EOF

cat /tmp/test1-$1
echo ""
cat /tmp/test1-output