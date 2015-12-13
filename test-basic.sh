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
		echo $var
	fi

}


################test ###########################
# helper function

echo "creating image file for test"

if [ -f "test.img" ]
then
	./mktest test.img
fi

echo "testing ls current dir"
output="/tmp/precompute"
output2="/tmp/real"

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
echo "TODO"

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
echo "file.A" >> $output
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
initialize $output
initialize puttest
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
echo "show puttest" >> $output
echo $NOFILE >> $output
echo $quitcmd >> $output

./homework -cmdline -image test.img << EOF > $output2
rm puttest
ls
show puttest
quit
EOF

check_result $output $output2 "remove"
# echo "testing mkdir"
# initialize $output
# echo "cmd> mkdir test" >> $output
# echo "cmd> ls" >> $output
# echo "dir1" >> $output
# echo "file.7" >> $output
# echo "file.A" >> $output
# echo "test" >> $output
# echo "cmd> ls test" >> $output

