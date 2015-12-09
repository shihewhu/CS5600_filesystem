#!/usr/bin/env bash


# get args
filename=$1
echo ${filename:6}
# make imgae
echo "making img"
./mktest foo.img

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

outputpath=""
if [[ "$filename" == "/dir1/"* ]]; then
	outputpath="/tmp/${filename:6}"
else
	outputpath="/tmp/$filename"
fi

./homework -cmdline -image foo.img << EOF > /tmp/test1-output
get $filename $outputpath
quit
EOF
echo "checking get function"
cksum=$(cksum $outputpath)
case $filename in
	"file.A" )
		# echo "check sum is $cksum"
		if [[ "$cksum" == "3509208153 1000 /tmp/file.A" ]]; then
			echo "get file.A succeeds"
		else
			echo "get file.A fails"
		fi
		;;
	"file.7" )
		# echo "check sum is $cksum"
		if [[ "$cksum" ==  "94780536 6644 /tmp/file.7" ]]; then
			echo "get file.7 succeeds"
		else
			echo "get file.7 fails"
		fi
		;;
	"/dir1/file.2" )
		if [[ "$cksum" ==  "3106598394 2012 /tmp/file.2" ]]; then
			echo "get /dir1/file.2 succeeds"
		else
			echo "get /dir1/file.2 fails"
		fi
		;;
	"/dir1/file.270" )
		# echo "check sum is $cksum"
		if [[ "$cksum" ==  "1733278825 276177 /tmp/file.270" ]]; then
			echo "get /dir1/file.270 succeeds"
		else
			echo "get /dir1/file.270 fails"
		fi
		;;
esac
echo "test1-output:"
cat /tmp/test1-output