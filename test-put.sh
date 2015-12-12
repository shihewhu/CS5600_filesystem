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
		echo "BITSIGHT1FACEBOOK3GOOGLE2" >> $2
	done
}

len=200
count=1
while [ $len -le 4000 ]
do
	#statements
	create_file $len "put-test-file.$count"
	len=$((len + 200))
	count=$((count + 1))
done

echo "starting test"

for i in `seq 1 $count`; do
	
	file-to-be-tested="put-test-file.$i"
	output-path="/tmp/$file-to-be-tested"
	cksum=$(cksum file-to-be-tested)
	./homework -cmdline -image foo.img << EOF
	put $file-to-be-tested
	get $file-to-be-tested $output-path
	quit
	EOF
	cksum2=$(cksum $output-path)
	if [[ cksum1 == cksum2 ]]; then
		echo "test case $i passed"
	else
		echo "test case $i failed"
	fi
done;

rm put-test-file.*





