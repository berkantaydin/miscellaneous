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

set -x
$QEMUCOMMAND -enable-kvm \
	-cpu host \
	-smp 2 \
	-m 2048 \
	-boot d \
	-drive file=$IMG,if=virtio,index=0,format=raw \
	-drive file=$TESTPATH/vm-aux/vm-swap-$IMGSTR,if=virtio,index=1,format=raw \
	-vga std \
	-kernel $KERN \
	-append 'root=/dev/vda ignore_loglevel debug console=hvc0' \
	-display none \
	-device virtio-serial \
	-chardev socket,path=/tmp/hvtty-$IMGSTR,server,nowait,id=hostconsole \
	-chardev socket,path=/tmp/console-$IMGSTR,server,nowait,id=hostlogin \
	-device virtioconsole,chardev=hostconsole,driver=virtconsole \
	-device virtioconsole,chardev=hostlogin,driver=virtconsole

if [ "$?" != "0" ]; then
	read
fi

echo "Cleaning up..."
rm -rf $TESTPATH/vm-aux/vm-swap-$IMGSTR
rm -rf /tmp/console-$IMGSTR
rm -rf /tmp/hvtty-$IMGSTR
