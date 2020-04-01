#!/bin/bash


count=16384

echo
echo Buffered Write:
div=1
for bs in 64 128 256 512 1024 2048 4096 8192; do
	echo -n "bs=${bs}K count=$(($count / $div)), "
	if [ -f $1 ]; then 
		echo 3 > /proc/sys/vm/drop_caches
	fi
	sync
	S=$(date +%s.%N)
	echo -n `dd if=/dev/zero of=$1 bs=${bs}K count=$(($count / $div)) 2>&1 | grep copied`
	sync
	E=$(date +%s.%N)
	echo -n ", "`echo "$bs*$(($count / $div))/1024/($E-$S)" | bc`; echo " MBytes/s";
	div=$(($div * 2))
done
: '
echo
echo Buffered Read:
div=1
for bs in 64 128 256 512 1024 2048 4096 8192; do
	echo -n "bs=${bs}K count=$(($count / $div)), "
	if [ -f $1 ]; then 
		echo 3 > /proc/sys/vm/drop_caches
	fi
	sync
	S=$(date +%s.%N)
	echo -n `dd if=$1 of=/dev/null bs=${bs}K count=$(($count / $div)) 2>&1 | grep copied`
	sync
	E=$(date +%s.%N)
	echo -n ", "`echo "$bs*$(($count / $div))/1024/($E-$S)" | bc`; echo " MBytes/s";
	div=$(($div * 2))
done
'
echo
echo Direct Write:
div=1
for bs in 64 128 256 512; do
	echo -n "bs=${bs}K count=$(($count / $div)) oflag=direct, "
	if [ -f $1 ]; then 
		echo 3 > /proc/sys/vm/drop_caches
	fi
	sync
	S=$(date +%s.%N)
	echo -n `dd if=/dev/zero of=$1 bs=${bs}K count=$(($count / $div)) oflag=direct 2>&1 | grep copied`
	sync
	E=$(date +%s.%N)
	echo -n ", "`echo "$bs*$(($count / $div))/1024/($E-$S)" | bc`; echo " MBytes/s";
	div=$(($div * 2))
done

for bs in 1 2 4 8 16 32 64 128 256; do
	echo -n "bs=${bs}M count=$(($count / $div)) oflag=direct, "
	if [ -f $1 ]; then 
		echo 3 > /proc/sys/vm/drop_caches
	fi
	sync
	S=$(date +%s.%N)
	echo -n `dd if=/dev/zero of=$1 bs=${bs}M count=$(($count / $div)) oflag=direct 2>&1 | grep copied`
	sync
	E=$(date +%s.%N)
	echo -n ", "`echo "$bs*$(($count / $div))/($E-$S)" | bc`; echo " MBytes/s";
	div=$(($div * 2))
done
: '
echo
echo Direct Read:
div=1
for bs in 64 128 256 512; do
	echo -n "bs=${bs}K count=$(($count / $div)) iflag=direct, "
	if [ -f $1 ]; then 
		echo 3 > /proc/sys/vm/drop_caches
	fi
	sync
	S=$(date +%s.%N)
	echo -n `dd if=$1 of=/dev/null bs=${bs}K count=$(($count / $div)) iflag=direct 2>&1 | grep copied`
	sync
	E=$(date +%s.%N)
	echo -n ", "`echo "$bs*$(($count / $div))/1024/($E-$S)" | bc`; echo " MBytes/s";
	div=$(($div * 2))
done

for bs in 1 2 4 8 16 32 64 128 256; do
	echo -n "bs=${bs}M count=$(($count / $div)) iflag=direct, "
	if [ -f $1 ]; then 
		echo 3 > /proc/sys/vm/drop_caches
	fi
	sync
	S=$(date +%s.%N)
	echo -n `dd if=$1 of=/dev/null bs=${bs}M count=$(($count / $div)) iflag=direct 2>&1 | grep copied`
	sync
	E=$(date +%s.%N)
	echo -n ", "`echo "$bs*$(($count / $div))/($E-$S)" | bc`; echo " MBytes/s";
	div=$(($div * 2))
done
'
