#!/bin/bash


# helper function
function create_file {
	len=$1
	for i in `seq 1 $len`; 
	do
		#statements
		echo "ABCDEFGHIJKLMNOPQRSTUVWXYZ" >> $2
	done
}



function check_result {
	diff_value=$(diff file.$1 /tmp/file.$1)
	if [[ $diff_value == "" ]]; then
		echo "test case $1 passed"
	else 
		echo "test case $1 failed, the difference is"
		echo "$diff_value"
	fi
}

function check_environment {
	echo "check exist of file.$1"
	if [[ -f "file.$1" ]]; then
		#statements
		rm file.$1
	fi
}

# get args

echo "cleaning files and compiling"
if [[ -f "test.img" ]]; then
	#statements
	rm test.img
fi

for i in `seq 1 3`; do
	#statements
	check_environment $i
done

./mktest test.img
make clean
make

echo "test 1: direct write and read"
create_file 100 file.1
echo "starting test"
./homework -cmdline -image test.img << EOF > /tmp/test-put-output-1
put file.1
get file.1 /tmp/file.1
quit
EOF

check_result $diff_value1 1

# echo "test case 2: height 1 tree write and read"
# create_file 1000 file.2
# echo "starting test"
# ./homework -cmdline -image test.img << EOF > /tmp/test-put-output-2
# put file.2
# get file.2 /tmp/file.2
# quit
# EOF
# diff_value2=$(diff file.2 /tmp/file.2)
# check_result $diff_value2 2

# echo "test case 3: height 2 tree write and read"
# create_file 10000 file.3
# echo "starting test"
# ./homework -cmdline -image test.img << EOF > /tmp/test-put-output-3
# put file.3
# get file.3 /tmp/file.3
# quit
# EOF
# diff_value3=$(diff file.3 /tmp/file.3)
# check_result $diff_value3 3

