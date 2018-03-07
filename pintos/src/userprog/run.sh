#!/bin/sh

export PATH=/home/h/happystep/cis520/proj2/pintos/src/utils:~/home/h/happystep/cis520/proj2/usr/local/bin:$PATH
export BXSHARE=/home/h/happystep/cis520/proj2/usr/local/share/bochs

clear
clear
echo entering directory /examples ...
cd ../examples/
make

echo Entering directory /userprog...
cd ../userprog/
make
#stop for user to press enter key
read -rp 'Press enter to continue.. ' second </dev/tty

echo entering directory /userprog/build...
cd build
pintos-mkdisk filesys.dsk --filesys-size=2
pintos -f -q
pintos -p ../../examples/echo -a echo -- -q 
pintos -q run 'echo x' 
