#!/bin/sh

echo << EOI
gdisk build/disk64.img << EOFIN
o
Y
n
1



t
ef00
w
Y
EOFIN

fdisk build/disk64.img << EOFIN
o
n
p
1


t
0c
a
w
EOFIN

EOI

sudo losetup -f build/disk64.img
losetup
sudo partprobe  `losetup -a | grep disk64.img | awk -F: '{print $1;}'`
LOOP_DEV_PATH=`losetup -a | grep disk64.img | awk -F: '{print $1;}'`p1
sudo mkfs.fat --mbr=n -F32  $LOOP_DEV_PATH

sudo losetup -d `losetup -a | grep disk64.img | awk -F: '{print $1;}'`
qemu-img convert -f raw -O vpc build/disk64.img build/disk64.vhd
exit

mkdir -p /tmp/disk64mnt
sudo mount $LOOP_DEV_PATH /tmp/disk64mnt
sudo mkdir -p /tmp/disk64mnt/EFI/BOOT
sudo cp build/bootx64.efi /tmp/disk64mnt/EFI/BOOT/BOOTX64.EFI
sudo umount /tmp/disk64mnt
sudo losetup -d `losetup -a | grep disk64.img | awk -F: '{print $1;}'`

qemu-img convert -f raw -O vpc build/disk64.img build/disk64.vhd

echo doneQ
