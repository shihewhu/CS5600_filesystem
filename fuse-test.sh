#!/usr/bin/env bash
######################error######################
function mkdirerror {

	echo "mkdir: cannot create directory ‘$1’: $2"

}
function toucherror {

	echo "touch: cannot touch ‘$1’: $2"
}
NOFILEORDIR="No such file or directory"
FILEEXISTS="File exists"
NOTDIR="Not a directory"
NOSPACE="No space left on device"
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
	yes '0 1 2 3 4 5 6 7' | fmt | head -c $1 > "/tmp/$2"
}

diffblsiztest() {
	for testfile in $(ls /tmp/fuse-test-file.*)
	do
		cp $testfile ./testdir$testfile
		dd bs=$1 if=./testdir$testfile | cmp - $testfile || fail Read $testfile with $1 block size failed
		echo "$(cat ./testdir$testfile | cksum)"
		echo "$(cat $testfile | cksum)"
		test "$(cat ./testdir$testfile | cksum)" = "$(cat $testfile | cksum)" || fail Read $testfile with $1 block size failed
		rm ./testdir$testfile
	done
}

echo "testing read"
echo "preparintg 4 files with different size"
makefile 0 fuse-test-file.0
makefile 4000 fuse-test-file.4
makefile 7000 fuse-test-file.7
makefile 10000 fuse-test-file.10
mkdir ./testdir/tmp
echo "test with 111 block size"
diffblsiztest 111
echo "test with 1024 block size"
diffblsiztest 1024
echo "test with 1517 block size"
diffblsiztest 1517
echo "test with 2701 block size"
diffblsiztest 2701

echo "cleaning"
rm -r ./testdir/tmp

echo "testing mknod"
rm -r /tmp/fuse-mknod-error*
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
test -d ./testdir/mkdir-test-dir
test "$(mkdir mkdir-test-dir)" = \
	"$()"
