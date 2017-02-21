#!/bin/bash

echo "What disk archive should I use?"
select STAGE in tarballs/*; do
	break;
done

echo "To which disk image am I extracting the filesystem?"
select IMG in vm-disks/*.img; do
	break;
done

echo "With which filesystem should I format the VM disk?"
select FSPROG in /sbin/mkfs.*; do
	break;
done

sudo -p "Enter thy password: " /bin/false

echo "Truncating new image file"
truncate -s 0 $IMG
truncate -s 10G $IMG

echo "Creating loopback device"
sudo losetup /dev/loop7 $IMG

echo "Formatting image"
sudo $FSPROG /dev/loop7

if [ ! -e mnt ]; then
	echo "(mnt/ does not exist, creating it)"
	mkdir mnt
fi

echo "Mounting image"
sudo mount /dev/loop7 mnt

echo "Unpacking $STAGE into $IMG"
sudo tar xfs $STAGE -C mnt

echo "Calling sync()"
sync

echo "Setting root password"
sudo sed -ri 's/root:\*/root:$6$9GmteSaW$O2bNtXoGfSG2nCo58ahxbU5frVnOWCrp3VXDAxY0tMCimP1puawnIJjajSaT0tNwG5XxvpN3GXKIeoFlvLoVo./g' mnt/etc/shadow

echo "Creating user calvinow"
sudo bash -c 'echo "calvinow:x:1000:1000::/home/calvinow:/bin/bash" >> mnt/etc/passwd'
sudo bash -c 'echo "calvinow:x:1000:" >> mnt/etc/group'
sudo bash -c 'echo "calvinow:$6$9GmteSaW$O2bNtXoGfSG2nCo58ahxbU5frVnOWCrp3VXDAxY0tMCimP1puawnIJjajSaT0tNwG5XxvpN3GXKIeoFlvLoVo.:15903:0:99999:7:::" >> mnt/etc/shadow'
sudo sed -ri 's/wheel:x:10:root/wheel:x:10:root,calvinow/g' mnt/etc/group
sudo mkdir mnt/home/calvinow
sudo chown 1000:1000 mnt/home/calvinow

echo "Fixing up /etc/fstab"
sudo bash -c "echo \"/dev/vda / ${FSPROG##'/sbin/mkfs.'} noatime 0 1\" > mnt/etc/fstab"
sudo bash -c 'echo "/dev/vdb none swap sw 0 0" >> mnt/etc/fstab'

echo "Enabling virtioconsole"
sudo sed -ri 's/^c/#c/g' mnt/etc/inittab
sudo bash -c 'echo "s0:12345:respawn:/sbin/agetty -L 115200 ttyS1 vt100 -a calvinow" >> mnt/etc/inittab'

echo "Setting timezones"
sudo bash -c 'echo "US/Pacific" > mnt/etc/timezone'
sudo cp mnt/usr/share/zoneinfo/US/Pacific mnt/etc/localtime

echo "Setting hostname"
sudo bash -c "echo \"hostname=\\\"${IMG%%".img"}\\\"\" > mnt/etc/conf.d/hostname"

echo "Unmounting image"
sudo umount mnt

echo "Destroying loopback device"
sudo losetup -d /dev/loop7
