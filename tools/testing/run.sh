#!/bin/bash

TERMINALCOMMAND="xfce4-terminal -H -e"
QEMUCOMMAND="qemu-system-x86_64"

echo "Which kernel should I use?"
select KERN in kernels/*; do
	break;
done

echo "Which disk image should I use as the root filesystem?"
select IMG in vm-disks/*; do
	break;
done

truncate -s 0 vm-aux/vm-swap.img
truncate -s 8G vm-aux/vm-swap.img
/sbin/mkswap vm-aux/vm-swap.img > /dev/null

IMGSTR="`basename $IMG`"

$TERMINALCOMMAND "bash -c \"sleep 1; socat /tmp/hvtty-$IMGSTR -,raw,icanon=0,echo=0; read\"" &
$TERMINALCOMMAND "bash -c \"sleep 1; socat /tmp/console-$IMGSTR -,raw,icanon=0,echo=0\"" &
$QEMUCOMMAND -enable-kvm \
	-cpu host \
	-smp 2 \
	-m 2048 \
	-boot d \
	-drive file=$IMG,if=virtio,index=0 \
	-drive file=vm-aux/vm-swap.img,if=virtio,index=1 \
	-vga std \
	-kernel $KERN \
	-append 'root=/dev/vda ignore_loglevel debug console=hvc0' \
	-display none \
	-device virtio-serial \
	-chardev socket,path=/tmp/hvtty-$IMGSTR,server,nowait,id=hostconsole \
	-chardev socket,path=/tmp/console-$IMGSTR,server,nowait,id=hostlogin \
	-device virtioconsole,chardev=hostconsole,driver=virtconsole \
	-device virtioconsole,chardev=hostlogin,driver=virtconsole

echo "Cleaning up..."
rm -rf /tmp/console-$IMGSTR
rm -rf /tmp/hvtty-$IMGSTR
