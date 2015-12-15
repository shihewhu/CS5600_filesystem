#!/usr/bin/env bash
######################error######################
function mkdirerror {

	echo "mkdir: cannot create directory ‘$1’: $2"

}
function toucherror {

	echo "touch: cannot touch ‘$1’: $2"
}
function rmerror {
	echo "rm: cannot remove ‘$1’: $2"
}
function rmdirerror {
	echo "rmdir: failed to remove ‘$1’: $2"
}
function mverror1 {
	echo "mv: cannot stat ‘$1’: $2"
}
function mverror2 {
	echo "mv: failed to access ‘$1’: $2"
}
function mverror3 {
	echo "mv: cannot move ‘$1’ to ‘$2’: $3"
}
NOFILEORDIR="No such file or directory"
FILEEXISTS="File exists"
NOTDIR="Not a directory"
NOSPACE="No space left on device"
ISDIR="Is a directory"
NOTEMPTY="Directory not empty"
######################helper function#############
fail(){
	echo FAILED: $*
	exit 1
}
######################prepare and run#############
echo "preparing for the running environment"
echo "ckecing mount"
test `fusermount -u ./testdir` 0

echo "compiling"

echo "making image for test"
./mktest test.img

if [ -d "testdir" ]
then
	rm -r testdir
fi
mkdir testdir
echo "running program"
./homework -image test.img ./testdir


echo "test for getattr"
test ! -e "./test/whatever/file" || fail Testing file not exits failed
test ! -d "./testdir/file.A/x" || fail Testing not dir is failed
test ! -e "./testdir/dir1/file.1" || fail Testing file not exits failed
test -d "./testdir/dir1/" || fail Testing is dir failed
test -f "./testdir/file.A" || fail Testing is file failed

echo "testing readdir"
for i in $(ls ./testdir)
do
	test -e "./testdir/$i" || fail ls Test is failed
done

test "$(ls -l ./testdir/file.A)" = \
	'-rwxrwxrwx 1 student student 1000 Jul 13  2012 ./testdir/file.A' || \
	fail ls -l a file ls failed


# making file with length helper function
makefile() {
	yes '0 1 2 3 4 5 6 7' | fmt | head -c $1 > "$2"
}

diffblsiztest_write() {
	for testfile in $(ls /tmp/fuse-test-file.*)
	do
		
		dd bs=$1 if=$testfile > ./testdir$testfile
		# echo "$(cat ./testdir$testfile | cksum)"
		# echo "$(cat $testfile | cksum)"
		cmp ./testdir$testfile $testfile || fail write $testfile failed
		test "$(cat ./testdir$testfile | cksum)" = "$(cat $testfile | cksum)" || fail wirte $testfile with $1 block size failed
		rm ./testdir$testfile
	done
}

echo "testing write"
echo "preparintg 4 files with different size"
makefile 0 /tmp/fuse-test-file.0
makefile 4000 /tmp/fuse-test-file.4
makefile 7000 /tmp/fuse-test-file.7
makefile 10000 /tmp/fuse-test-file.10
makefile 263000 /tmp/fuse-test-file.262
makefile 510000 /tmp/fuse-test-file.510
mkdir ./testdir/tmp
echo "test with 111 block size"
diffblsiztest_write 111
echo "test with 111 block size passed"
echo "test with 1024 block size"
diffblsiztest_write 1024
echo "test with 1024 block size passed"
echo "test with 1517 block size"
diffblsiztest_write 1517
echo "test with 1517 block size passed"
echo "test with 2701 block size"
diffblsiztest_write 2701
echo "test with 2701 block size passed"

echo "stress test for write"



echo "cleaning"
rm -r ./testdir/tmp


# helper function
diffblsizread() {
	blsizarry=(111 1024 1517 2701)
	for i in $blsizarry
	do
		dd bs=$i if="./testdir/$1" > "/tmp/$1"
		echo "$(cat ./testdir/$1 | cksum)"
		echo "$(cat /tmp/$1 | cksum)"
		cmp ./testdir/$1 /tmp/$1 || fail read ./testdir/$1 failed
		test "$(cat ./testdir/$1 | cksum)" = "$(cat /tmp/$1 | cksum)" || fail read ./testdir/$1 with $1 block size failed
		rm ./testdir/$1
	done
}
echo "test for read"

makefile 0 ./testdir/test-read.0
diffblsizread test-read.0
makefile 4000 ./testdir/test-read.4
diffblsizread test-read.4
makefile 7000 ./testdir/test-read.7
diffblsizread test-read.7
makefile 10000 ./testdir/test-read.10
diffblsizread test-read.10
makefile 263000 ./testdir/test-read.26
diffblsizread test-read.26
makefile 710000 ./testdir/test-read.510
diffblsizread test-read.510
echo "test for read passed"

echo "testing mknod"
if [ -f /tmp/fuse-mkdir-error.* ]
then
	rm -r /tmp/fuse-mknod-error*
fi
badpath1="./testdir/whatever/x" # whatever doens't exist
badpath2="./testdir/file.A/x" # file.A is not a directory
badpath3="./testdir/dir1/file.0" # file.0 exists is a file
badpath4="./testdir/dir1/" # dir1 exists is a directory

touch $badpath1 2> /tmp/fuse-mknod-error.1
touch $badpath2 2> /tmp/fuse-mknod-error.2
# touch $badpath3 2> /tmp/fuse-mknod-error.3
# touch $badpath4 2> /tmp/fuse-mknod-error.4

test "$(cat /tmp/fuse-mknod-error.1)" = \
	"$(toucherror $badpath1 "$NOFILEORDIR")" || \
	fail fault-tolerance Test 1 failed

test "$(cat /tmp/fuse-mknod-error.2)" = \
	"$(toucherror $badpath2 "$NOTDIR")" || \
	fail fault-tolerance Test 2 failed
if [ -f ./testdir/fuse-mknod-test-file ]
then
	rm ./testdir/fuse-mknod-test-file
fi
touch ./testdir/fuse-mknod-test-file
test -f ./testdir/fuse-mknod-test-file || fail Test mknod a good path failed
echo "stress test for mknod"
echo "cleaning for test"
echo "creating 32 inodes"
for i in `seq 5 32`
do
	touch ./testdir/stress-test-file.$i
done
touch ./testdir/stress-test-file.33 2> /tmp/stress-test-error
test "$(cat /tmp/stress-test-error)" = \
	"$(toucherror ./testdir/stress-test-file.33 "$NOSPACE")" || \
	fail stress Test For mknod failed
rm -r ./testdir/stress*

echo "testing mkdir"
if [ -f /tmp/fuse-mkdir-error* ]
then
	rm -r /tmp/fuse-mkdir-error*
fi
badpath1="./testdir/whatever/x" # whatever doens't exist
badpath2="./testdir/file.A/x" # file.A is not a directory
badpath3="./testdir/dir1/file.0" # file.0 exists is a file
badpath4="./testdir/dir1/" # dir1 exists is a directory

mkdir $badpath1 2> /tmp/fuse-mkdir-error.1
mkdir $badpath2 2> /tmp/fuse-mkdir-error.2
mkdir $badpath3 2> /tmp/fuse-mkdir-error.3
mkdir $badpath4 2> /tmp/fuse-mkdir-error.4

test "$(cat /tmp/fuse-mkdir-error.1)" = \
	"$(mkdirerror $badpath1 "$NOFILEORDIR")" || \
	fail mkdir $badpath1 Test failed
test "$(cat /tmp/fuse-mkdir-error.2)" = \
	"$(mkdirerror $badpath2 "$NOTDIR")" || \
	fail mkdir $badpath2 Test failed
test "$(cat /tmp/fuse-mkdir-error.3)" = \
	"$(mkdirerror $badpath3 "$FILEEXISTS")" || \
	fail mkdir $badpath3 Test failed
test "$(cat /tmp/fuse-mkdir-error.4)" = \
	"$(mkdirerror $badpath4 "$FILEEXISTS")" || \
	fail mkdir $badpath4 Test failed
rm -r /tmp/fuse-mkdir-error*

if [ -d mkdir-test-dir ]
then
	rm -r mkdir-test-dir
fi
mkdir ./testdir/mkdir-test-dir
test -d ./testdir/mkdir-test-dir || fail mkdir a good path failed
echo "stress test for mkdir"
echo "fill the dir"
for i in `seq 6 32`
do
	mkdir ./testdir/mkdir-stress-test.$i
done
mkdir ./testdir/mkdir-stress-test.33 2> /tmp/mkdir-stress-test-error
test "$(cat /tmp/mkdir-stress-test-error)" = \
	"$(mkdirerror ./testdir/mkdir-stress-test.33 "$NOSPACE")" || \
	fail stress Test For mkdir failed

rm -r /tmp/mkdir*
rm -r ./testdir/mkdir-stress-test.*

echo "testing unlink"
badpath1="./testdir/whatever/x" # whatever doens't exist
badpath2="./testdir/file.A/x" # file.A is not a directory
badpath3="./testdir/dir1/file.1" # file.1 doesn't exist
badpath4="./testdir/dir1/" # dir1 exists is a directory

rm $badpath1 2> /tmp/rm-test-error.1
rm $badpath2 2> /tmp/rm-test-error.2
rm $badpath3 2> /tmp/rm-test-error.3
rm $badpath4 2> /tmp/rm-test-error.4

test "$(cat /tmp/rm-test-error.1)" = \
	"$(rmerror $badpath1 "$NOFILEORDIR")" || \
	fail fault-tolerance Test For rm $badpath1 failed
test "$(cat /tmp/rm-test-error.2)" = \
	"$(rmerror $badpath2 "$NOTDIR")" || \
	fail fault-tolerance Test For rm $badpath2 failed
test "$(cat /tmp/rm-test-error.3)" = \
	"$(rmerror $badpath3 "$NOFILEORDIR")" || \
	fail fault-tolerance Test For rm $badpath3 failed
test "$(cat /tmp/rm-test-error.4)" = \
	"$(rmerror $badpath4 "$ISDIR")" || \
	fail fault-tolerance Test For rm $badpath4 failed
if [ ! -f ./testdir/file-For-rm-test ]
then
	echo "hello" > ./testdir/file-For-rm-test
fi
rm ./testdir/file-For-rm-test
test ! -e ./testdir/file-For-rm-test || fail remove a good path failed
# makefile 0 fuse-test-file.0
# makefile 4000 fuse-test-file.4
# makefile 7000 fuse-test-file.7
# makefile 10000 fuse-test-file.10
# makefile 263000 fuse-test-file.262
if [ -d ./testdir/tmp ]
then
	rm -r ./testdir/tmp
fi
mkdir ./testdir/tmp
for file in $(ls /tmp/fuse-test-file.*)
do
	cp $file ./testdir$file
	rm ./testdir$file
	test ! -e ./testdir$file || fail rm ./testdir$file 1 failed
	test "$(./read-img test.img | tail -2 | wc --words)" = 4 || fail rm ./testdir$file  2 failed
done
echo "unlink test passed"
echo "testing truncate"
if [ -d ./testdir/tmp ]
then
	rm -r ./testdir/tmp
fi
mkdir ./testdir/tmp
for file in $(ls /tmp/fuse-test-file.*)
do
	cp $file ./testdir$file
	truncate -s 0 ./testdir$file
	test -e ./testdir$file || fail truncate ./testdir$file failed
	test "$(stat -c%s ./testdir$file)" = "0" || fail truncate .testdir$file 2 failed, no zeroify size
	# ./read-img test.img
	test "$(./read-img test.img | tail -2 | wc --words)" = 4 || \
		fail truncate ./testdir$file failed lost block
	rm ./testdir$file
done
echo "truncate test passed"
echo "testing rmdir"
badpath1="./testdir/whatever/x" # whatever doens't exist
badpath2="./testdir/file.A/x" # file.A is not a directory
badpath3="./testdir/dir1/file.1" # file.1 doesn't exist
badpath4="./testdir/dir1/file.0" # file.0 exists is a file

rmdir $badpath1 2> /tmp/rmdir-error.1
rmdir $badpath2 2> /tmp/rmdir-error.2
rmdir $badpath3 2> /tmp/rmdir-error.3
rmdir $badpath4 2> /tmp/rmdir-error.4

test "$(cat /tmp/rmdir-error.1)" = \
	"$(rmdirerror $badpath1 "$NOFILEORDIR")" || \
	fail fault-tolerance Test For rmdir $badpath1 failed
test "$(cat /tmp/rmdir-error.2)" = \
	"$(rmdirerror $badpath2 "$NOTDIR")" || \
	fail fault-tolerance Test For rmdir $badpath2 failed
test "$(cat /tmp/rmdir-error.3)" = \
	"$(rmdirerror $badpath3 "$NOFILEORDIR")" || \
	fail fault-tolerance Test For rmdir $badpath3 failed
test "$(cat /tmp/rmdir-error.4)" = \
	"$(rmdirerror $badpath4 "$NOTDIR")" || \
	fail fault-tolerance Test For rmdir $badpath4 failed
rm -r /tmp/rmdir-error.*
rmdir ./testdir/dir1 2> /tmp/rmdir-error.5
test "$(cat /tmp/rmdir-error.5)" = \
	"$(rmdirerror ./testdir/dir1 "$NOTEMPTY")" || \
	fail fault-tolerance Test For rmdir a Non empty dir
if [ -d ./testdir/dir-tobe-test ]
then
	rm -r ./testdir/dir-tobe-test
fi
mkdir ./testdir/dir-tobe-test
rmdir ./testdir/dir-tobe-test
test ! -d ./testdir/dir-tobe-test || fail rmdir a good path failed
echo "rmdir test passed"

echo "test chmod"

chmod 754 ./testdir/file.A
test "$(ls -l ./testdir/file.A)" = \
	'-rwxr-xr-- 1 student student 1000 Jul 13  2012 ./testdir/file.A' || \
	fail chmod Test failed
echo "chmod test passsed"

echo "utime test"
touch -d 'Jan 01 2000' ./testdir/file.A
test "$(ls -l ./testdir/file.A)" = \
	'-rwxr-xr-- 1 student student 1000 Jan  1  2000 ./testdir/file.A' || \
	fail utime Test failed
echo "utime test passed"

echo "testing rename"
mv ./testdir/whatever ./testdir/anything 2> /tmp/rename-error.1
mv ./testdir/file.A/x ./testdir/file.A/y 2> /tmp/rename-error.2
mv ./testdir/dir1/file.0 ./testdir/dir1/file.2 2> /tmp/rename-error.3
mv ./testdir/file.A ./testdir/file.7 2> /tmp/rename-error.4
mv ./testdir/file.A ./testdir/dir1 2> /tmp/rename-error.5

test "$(cat /tmp/rename-error.1)" = \
	"$(mverror1 ./testdir/whatever "$NOFILEORDIR")" || \
	fail move ./test/whatever Test failed
test "$(cat /tmp/rename-error.2)" = \
	"$(mverror2 ./testdir/file.A/y "$NOTDIR")" || \
	fail move ./testdir/file.A/y Test failed
test "$(cat /tmp/rename-error.3)" = \
	"$(mverror3 ./testdir/dir1/file.0 ./testdir/dir1/file.2 "$FILEEXISTS")" || \
	fail move ./testdir/file.A/file.0 failed
mv ./testdir/file.A ./testdir/file.8
test ! -f ./testdir/file.A || fail move a file failed source still exists
test -f ./testdir/file.8 || fail move a file failed no dest file
echo "rename test passed"
echo "stress test 1 for write"
makefile 1024000 /tmp/bigfile1
makefile 2048000 /tmp/bigfile2
# cp /tmp/bigfile testdir/ 2> /dev/null
# slice="$(wc --bytes testdir/bigfile)"
cp /tmp/bigfile1 testdir/ 2> /dev/null
# echo "$(wc --bytes testdir/bigfile1)"
slice1=$(stat -c%s testdir/bigfile1)
rm testdir/bigfile1
cp /tmp/bigfile2 testdir/ 2> /dev/null
slice2=$(stat -c%s testdir/bigfile2)
test "$slice1" = "$slice2" || fail stress Test For write failed
echo "stress test 1 for write passed"
echo "stress test 2 for write"
rm /tmp/bigfile2
cat testdir/bigfile2 > /tmp/bigfile2
cmp testdir/bigfile2 /tmp/bigfile2 || fail stree Test  2 For write failed
echo "stress test 2 for write passed"
echo "stress test 3 for write"
rm testdir/bigfile2

for i in `seq 1 `
do
	cp /tmp/bigfile2 testdir/truncatedfile1 2> /dev/null
	cp testdir/truncatedfile1 /tmp/truncatedfile1
	rm testdir/truncatedfile1
	cp /tmp/truncatedfile1 testdir/truncatedfile2 
	cp testdir/truncatedfile2 /tmp/truncatedfile2
	rm testdir/truncatedfile2
	cmp /tmp/truncatedfile1 /tmp/truncatedfile2 || fail stress Test 3 For write failed
	rm /tmp/truncatedfile1 /tmp/truncatedfile2
done
echo "stress test 3 for write passed"
echo "all tests passed"
rm /tmp/*test*
fusermount -u testdir
rm -r testdir

