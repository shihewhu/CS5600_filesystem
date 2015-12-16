#!/usr/bin/env bash

# ./homework -cmdline -image test.img << EOF > /tmp/ls-out-put
# ls
# quit
# EOF

################## global value ##################
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
# helper function
make clean
make COVERAGE=1
echo "creating image file for test"

if [ -f "test.img" ]
then
	./mktest test.img
fi



echo "initial test"





./mktest test.img
echo "testing ls current dir"
initialize $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.7" >> $output
echo "file.A" >> $output
echo "cmd> quit" >> $output

./homework -cmdline -image test.img << EOF > $output2
ls
quit
EOF

check_result $output $output2 "ls current dir"


echo "testing ls a file"
initialize $output
echo "cmd> ls file.A" >> $output
echo $NOTDIR >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
ls file.A
quit
EOF

check_result $output $output2 "ls a file"

echo "testing ls a dir path"
initialize $output
echo "cmd> ls /dir1/" >> $output
echo "file.0" >> $output
echo "file.2" >> $output
echo "file.270" >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
ls /dir1/
quit
EOF

check_result $output $output2 "ls a dir path"

echo "testing ls-l current dir"
initialize $output
echo "cmd> ls-l /whatever/file" >> $output
echo $NOFILE >> $output
echo "cmd> ls-l /file.A/file" >> $output
echo $NOFILE >> $output
echo "cmd> ls-l /dir1/file.11" >> $output
echo $NOFILE >> $output
echo "cmd> ls-l file.A" >> $output
echo "/file.A -rwxrwxrwx 1000 1" >> $output
echo "cmd> ls-l /dir1/" >> $output
echo "file.0 -rwxrwxrwx 0 1" >> $output
echo "file.2 -rwxrwxrwx 2012 2" >> $output
echo "file.270 -rwxrwxrwx 276177 270" >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
ls-l /whatever/file
ls-l /file.A/file
ls-l /dir1/file.11
ls-l file.A
ls-l /dir1/
quit
EOF

check_result $output $output2 "ls-l"


echo "testing rename a file"
initialize $output
echo "cmd> rename file.7 file.8" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.8" >> $output
echo "file.A" >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
rename file.7 file.8
ls
quit
EOF

check_result $output $output2 "rename a file"

echo "testing rename a file with shorter name"
initialize $output
echo "cmd> rename file.8 f" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "f" >> $output
echo "file.A" >> $outputecho "/file.A -rwxrwxrwx 1000 1" >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
rename file.8 f
ls
quit
EOF

check_result $output $output2 "rename file with shorter name"


echo "testing rename a file with longer name"
initialize $output
echo "cmd> rename f file.7" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.7" >> $output
echo "file.A" >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
rename f file.7
ls
quit
EOF

check_result $output $output2 "rename file with longer name"


echo "testing basic put file"
touch puttest
initialize $output
echo "testing basic put file" > puttest
echo "cmd> put puttest" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.7" >> $output
echo "file.A" >> $output
echo "puttest" >> $output
echo "cmd> show puttest" >> $output
cat puttest >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
put puttest
ls
show puttest
quit
EOF

check_result $output $output2 "baisc put file"

echo "testing remove"
initialize $output
echo "cmd> rm puttest" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.7" >> $output
echo "file.A" >> $output
echo "cmd> show puttest" >> $output
echo $NOFILE >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
rm puttest
ls
show puttest
quit
EOF

check_result $output $output2 "remove"

echo "testing mkdir"
initialize $output
echo "cmd> mkdir dir1" >> $output # bad path dir exits
echo $FILEEXISTS >> $output
echo "cmd> mkdir file.7" >> $output # bad path file exits
echo $FILEEXISTS >> $output
echo "cmd> mkdir /file.7/test/" >> $output # bath path
echo $FILEEXISTS >> $output
echo "cmd> mkdir /whatever/test/" >> $output # bath path
echo $FILEEXISTS >> $output
echo "cmd> mkdir test" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.7" >> $output
echo "file.A" >> $output
echo "test" >> $output
echo "cmd> ls test" >> $output
echo "cmd> cd test" >> $output
echo "cmd> put puttest" >> $output
echo "cmd> ls" >> $output
echo "puttest" >> $output
echo "cmd> show puttest" >> $output
cat puttest >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
mkdir dir1
mkdir file.7
mkdir /file.7/test/
mkdir /whatever/test/
mkdir test
ls
ls test
cd test
put puttest
ls
show puttest
quit
EOF

check_result $output $output2 "mkdir"

echo "testing remove dir" 
initialize $output
echo "cmd> rmdir /whatever/test/" >> $output
echo $NOFILE >> $output
echo "cmd> rmdir /file.7/test/" >> $output
echo $NOFILE >> $output
echo "cmd> rmdir /dir1/test/" >> $output
echo $NOFILE >> $output
echo "cmd> rmdir /dir1/file.0/" >> $output
echo $NOTDIR >> $output
echo "cmd> rmdir test" >> $output
echo $NOTEMPTY >> $output
echo "cmd> rm test/puttest" >> $output
echo "cmd> ls test" >> $output
echo "cmd> rmdir test" >> $output
echo "cmd> ls" >> $output
echo "dir1" >> $output
echo "file.7" >> $output
echo "file.A" >> $output
echo "cmd> ls test" >> $output
echo $NOFILE >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
rmdir /whatever/test/
rmdir /file.7/test/
rmdir /dir1/test/
rmdir /dir1/file.0/
rmdir test
rm test/puttest
ls test
rmdir test
ls
ls test
quit
EOF

check_result $output $output2 "remove dir"


echo "testing unlink"
initialize $output
echo "cmd> rm /whatever/file/" >> $output
echo $NOFILE >> $output
echo "cmd> rm /file.7/file/" >> $output
echo $NOFILE >> $output
echo "cmd> rm /dir1/file.11" >> $output
echo $NOFILE >> $output
echo "cmd> rm /dir1/" >> $output
echo $ISDIR >> $output
echo "cmd> rm /dir1/file.0" >> $output
echo "cmd> ls /dir1/" >> $output
echo "file.2" >> $output
echo "file.270" >> $output
echo "cmd> show /dir1/file.0" >> $output
echo $NOFILE >> $output 
echo "cmd> rm /dir1/file.2" >> $output
echo "cmd> ls /dir1/" >> $output

echo "file.270" >> $output
echo "cmd> show /dir1/file.2" >> $output
echo $NOFILE >> $output
echo "cmd> rm /dir1/file.270" >> $output
echo "cmd> ls /dir1/" >> $output
echo "cmd> show /dir1/file.270" >> $output
echo $NOFILE >> $output
echo $quitcmd >> $output


./homework -cmdline -image test.img << EOF > $output2
rm /whatever/file/
rm /file.7/file/
rm /dir1/file.11
rm /dir1/
rm /dir1/file.0
ls /dir1/
show /dir1/file.0
rm /dir1/file.2
ls /dir1/
show /dir1/file.2
rm /dir1/file.270
ls /dir1/
show /dir1/file.270
quit
EOF

check_result $output $output2 "unlink"

rm $output
rm $output2


echo "testing chmod"
initialize $output
echo "cmd> chmod 754 file.A" >> $output
echo "cmd> ls-l file.A" >> $output
echo "/file.A -rwxr-xr-- 1000 1" >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
chmod 754 file.A
ls-l file.A
quit
EOF

check_result $output $output2 "change mode"


########################### coverage #############
# echo "coverage is:"
# gcov homework
