#!/usr/bin/env bash

# get args

echo "cleaning files"
if [[ -f "test.img" ]]; then
	#statements
	rm test.img
fi
echo "making image for test"
./mktest test.img

echo "compiling"
make clean
make

echo "making test files"
#helper function
function create_file {
	if [[ -f $2 ]]; then
		#clean file
		echo "" > $2
	fi
	for i in $(seq 1 $1) ; do
		random_number=$(( ( RANDOM % 1000 )  + 1 ))
		echo $random_number >> $2
	done
}

len=2000
count=1
while [ $len -le 140000 ]
do
	#statements
	create_file $len "put-test-file.$count"
	len=$((len + 2000))
	count=$((count + 1))
done

count=$((count - 1))
echo "starting test"

for i in `seq 1 $count`; do
	testing_file="put-test-file.$i"
	output_file="/tmp/$testing_file"
	cksum1=$(cksum $testing_file)
	./mktest test.img
	./homework -cmdline -image test.img << EOF
	put $testing_file
	get $testing_file $output_file
	quit
EOF
	cksum2=$(cksum $output_file)
	IFS=' ' read -r -a cksum1 <<< "$cksum1"
	IFS=' ' read -r -a cksum2 <<< "$cksum2"
	ls -l put-test-file.$i
	if [[ "${cksum1[0]}" == "${cksum2[0]}" ]]; then
		echo "test case $i passed"
	else
		echo "test case $i failed"
		echo $cksum1
		echo $cksum2

	fi
done;

# cat put-test-file.1
rm put-test-file.*
rm /tmp/put-test-file.*

echo "stress test"







