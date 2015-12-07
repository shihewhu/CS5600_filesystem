#!/usr/bin/env bash

# get args
filename=$1

echo "compiling"
make

echo "checking test enviroment"
if [[ -f /tmp/test1-output ]]; then 
		rm /tmp/test1-output
		# echo "remove test1-output"
fi
if [[ -f /tmp/test1-$filename ]]; then 
		rm /tmp/test1-$filename
		# echo "remove file"
fi
./homework -cmdline -image foo.img << EOF > /tmp/test1-output
get $filename /tmp/test1-$filename
quit
EOF
echo "checking get function"
cksum=$(cksum /tmp/test1-$filename)
case $filename in
	"file.A" )
		# echo "check sum is $cksum"
		if [[ "$cksum" == "3509208153 1000 /tmp/test1-file.A" ]]; then
			echo "get file.A succeeds"
		else
			echo "get file.A fails"
		fi
		;;
	"file.7" )
		# echo "check sum is $cksum"
		if [[ "$cksum" ==  "94780536 6644 /tmp/test1-file.7" ]]; then
			echo "get file.7 succeeds"
		else
			echo "get file.7 fails"
		fi
esac
echo "test1-output:"
cat /tmp/test1-output