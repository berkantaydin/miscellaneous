#!/bin/bash

QEMUCOMMAND="$1"
TESTPATH="$2"
IMG="$3"
KERN="$4"

IMGSTR="`basename $IMG`"

truncate -s 0 $TESTPATH/vm-aux/vm-swap-$IMGSTR
truncate -s 1G $TESTPATH/vm-aux/vm-swap-$IMGSTR
/sbin/mkswap -f $TESTPATH/vm-aux/vm-swap-$IMGSTR > /dev/null

tmux split-window -p 85 -v "$TESTPATH/start-virtioconsoles.sh \"$IMGSTR\"" &
sleep 1

set -x
$QEMUCOMMAND -kernel $KERN -enable-kvm -cpu host -smp 2 -m 2048 -boot d \
	-nographic -vga none -display none \
	-append 'root=/dev/vda ignore_loglevel debug earlyprintk=serial,ttyS0,115200 console=ttyS0,115200' \
	-drive file=$IMG,if=virtio,index=0,format=raw \
	-drive file=$TESTPATH/vm-aux/vm-swap-$IMGSTR,if=virtio,index=1,format=raw \
	-chardev socket,path=/tmp/console-$IMGSTR,id=hostconsole \
	-serial chardev:hostconsole \
	-chardev socket,path=/tmp/login-$IMGSTR,id=hostlogin \
	-serial chardev:hostlogin


if [ "$?" != "0" ]; then
	read
fi

echo "Cleaning up..."
rm -rf $TESTPATH/vm-aux/vm-swap-$IMGSTR
rm -rf /tmp/console-$IMGSTR
rm -rf /tmp/login-$IMGSTR
