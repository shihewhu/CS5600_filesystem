#!/usr/bin/env bash

#######################global variable#####################
NOSPACE="error: No space left on device"
quitcmd="cmd> quit"
NOTDIR="error: Not a directory"
NOFILE="error: No such file or directory"
FILEEXISTS="error: File exists"
NOTEMPTY="error: Directory not empty"
ISDIR="error: Is a directory"
output="/tmp/precompute"
output2="/tmp/real"
################## helper function ###############
function initialize {
	if [ -f $1 ] 
	then
		rm $1
	fi
		touch $1
		head="read/write block size: 1000"
		echo $head >> $1
}

function check_result {
	var=$(diff $1 $2)
	if [ "$var" == "" ]
	then 
		echo "test $3 passed"
	else
		echo "test $3 failed"
		cat $output
		echo "#######"
		cat $output2
	fi

}


################test ###########################
#######################test################################
echo "stress test for write/read"
echo "cleaning files"
if [[ -f "test.img" ]]; then
	#statements
	rm test.img
fi
echo "making image for test"
./mktest test.img

echo "making test files"
#help function
function create_file {
	# if [[ -f $2 ]]; then
	# 	#clean file
	# 	echo "" > $2
	# fi
	# for i in $(seq 1 $1) ; do
	# 	random_number=$(( ( RAsNDOM % 1000 )  + 1 ))
	# 	echo $random_number >> $2
	# done
	yes '$random_number' | fmt | head -c $1 > "$2"
}


create_file 0 put-test-file.1
create_file 1000 put-test-file.2
create_file 1024 put-test-file.3
create_file 1571 put-test-file.4
create_file 70000 put-test-file.5
create_file 100000 put-test-file.6
create_file 262000 put-test-file.7
create_file 500000 put-test-file.8
echo "starting test"

for i in $(seq 1 8); do
	testing_file="put-test-file.$i"
	output_file="/tmp/put-test-file.$i"
	cksum1=$(cksum $testing_file)
	./mktest test.img
	./homework -cmdline -image test.img << EOF > /dev/null
	put $testing_file
	get $testing_file $output_file
	quit
EOF
	cksum2=$(cksum $output_file)
	IFS=' ' read -r -a cksum1 <<< "$cksum1"
	IFS=' ' read -r -a cksum2 <<< "$cksum2"
	if [[ "${cksum1[0]}" == "${cksum2[0]}" ]]; then
		echo "test case $i passed"
	else
		echo "test case $i failed"
		echo $cksum1
		echo $cksum2

	fi
done;

# cat put-test-file.1
# rm put-test-file.*
# rm /tmp/put-test-file.*


echo "making a empty img"
./mktest test.img
./homework -cmdline -image test.img << EOF > NULL
rm file.A
rm file.7
rm /dir1/file.0
rm /dir1/file.2
rm /dir1/file.270
rmdir dir1
EOF

echo "stress test for mknod"
touch tmpfile
initialize $output
for i in `seq 1 32`;
do
	echo "cmd> put tmpfile tmpfile.$i" >> $output
done
echo "cmd> put tmpfile tmpfile.33" >> $output
echo $NOSPACE >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
put tmpfile tmpfile.1
put tmpfile tmpfile.2
put tmpfile tmpfile.3
put tmpfile tmpfile.4
put tmpfile tmpfile.5
put tmpfile tmpfile.6
put tmpfile tmpfile.7
put tmpfile tmpfile.8
put tmpfile tmpfile.9
put tmpfile tmpfile.10
put tmpfile tmpfile.11
put tmpfile tmpfile.12
put tmpfile tmpfile.13
put tmpfile tmpfile.14
put tmpfile tmpfile.15
put tmpfile tmpfile.16
put tmpfile tmpfile.17
put tmpfile tmpfile.18
put tmpfile tmpfile.19
put tmpfile tmpfile.20
put tmpfile tmpfile.21
put tmpfile tmpfile.22
put tmpfile tmpfile.23
put tmpfile tmpfile.24
put tmpfile tmpfile.25
put tmpfile tmpfile.26
put tmpfile tmpfile.27
put tmpfile tmpfile.28
put tmpfile tmpfile.29
put tmpfile tmpfile.30
put tmpfile tmpfile.31
put tmpfile tmpfile.32
put tmpfile tmpfile.33
quit
EOF

check_result $output $output2 "stress test for mknod"

# echo "stress testing for mkdir"
# echo "makeing empty img for test"
# ./mktest test.img
# ./homework -cmdline -image test.img << EOF
# rm file.A
# rm file.7
# rm /dir1/file.0
# rm /dir1/file.2
# rm /dir1/file.270
# rmdir dir1
# EOF

rm $output
rm $output2

echo "making a empty img"
./mktest test.img
./homework -cmdline -image test.img << EOF > NULL
rm file.A
rm file.7
rm /dir1/file.0
rm /dir1/file.2
rm /dir1/file.270
rmdir dir1
EOF

echo "stress test for mkdir"
touch tmpfile
initialize $output
for i in `seq 1 32`;
do
	echo "cmd> mkdir dir.$i" >> $output
done
echo "cmd> mkdir dir.33" >> $output
echo $NOSPACE >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
mkdir dir.1
mkdir dir.2
mkdir dir.3
mkdir dir.4
mkdir dir.5
mkdir dir.6
mkdir dir.7
mkdir dir.8
mkdir dir.9
mkdir dir.10
mkdir dir.11
mkdir dir.12
mkdir dir.13
mkdir dir.14
mkdir dir.15
mkdir dir.16
mkdir dir.17
mkdir dir.18
mkdir dir.19
mkdir dir.20
mkdir dir.21
mkdir dir.22
mkdir dir.23
mkdir dir.24
mkdir dir.25
mkdir dir.26
mkdir dir.27
mkdir dir.28
mkdir dir.29
mkdir dir.30
mkdir dir.31
mkdir dir.32
mkdir dir.33
quit
EOF

check_result $output $output2 "stress test for mkdir"

# echo "stress testing for mkdir"
# echo "makeing empty img for test"
# ./mktest test.img
# ./homework -cmdline -image test.img << EOF
# rm file.A
# rm file.7
# rm /dir1/file.0
# rm /dir1/file.2
# rm /dir1/file.270
# rmdir dir1
# EOF

rm $output
rm $output2

