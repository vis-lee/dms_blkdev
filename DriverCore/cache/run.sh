insmod ccma_nbd.ko
mkfs.ext2 /dev/ccmaa
mount /dev/ccmaa /mnt/
ls /mnt/
cp ccma_nbd.c /mnt/
cat /mnt/ccma_nbd.c
umount /mnt/
rmmod ccma_nbd
