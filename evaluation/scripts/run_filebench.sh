#!/bin/bash

FS=$1
MOUNT_POINT=$2
TEST=$3
OUTPUT_DIR=$4
PM_DEVICE=$5

if [ -z $FS] | [ -z $MOUNT_POINT ] | [ -z $TEST] | [ -z $OUTPUT_DIR ] | [ -z $PM_DEVICE ]; then 
    echo "Usage: run_filebench.sh fs mountpoint test output_dir pm_device"
    exit 1
fi
mkdir -p $MOUNT_POINT

iterations=5
filename=$OUTPUT_DIR/${FS}/filebench/${TEST}
mkdir -p $filename


echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
for i in $(seq $iterations)
do
    if [ $FS = "squirrelfs" ]; then 
        sudo -E insmod ../linux/fs/squirrelfs/squirrelfs.ko; sudo mount -t squirrelfs -o init $PM_DEVICE $MOUNT_POINT/
    elif [ $FS = "nova" ]; then 
        sudo -E insmod ../linux/fs/nova/nova.ko; sudo mount -t NOVA -o init $PM_DEVICE $MOUNT_POINT/
    elif [ $FS = "winefs" ]; then 
        sudo -E insmod ../linux/fs/winefs/winefs.ko; sudo mount -t winefs -o init $PM_DEVICE $MOUNT_POINT/
    elif [ $FS = "ext4" ]; then 
        yes | sudo mkfs.ext4 $PM_DEVICE 
        sudo -E mount -t ext4 -o dax $PM_DEVICE $MOUNT_POINT/
    elif [ $FS = "arckfs" ]; then 
        sudo ndctl create-namespace -f -e namespace0.0 --mode=devdax
        sudo insmod arckfs/kfs/sufs.ko pm_nr=1
        sudo chmod 666 /dev/supremefs
        sudo arckfs/fsutils/init
    fi 

    if [ $FS = "arckfs" ]; then 
        sudo -E numactl --cpunodebind=1 --membind=1 filebench/filebench-sufs -f filebench/workloads/$TEST.f > ${filename}/Run$i
        sudo rmmod sufs
    else 
        sudo -E numactl --membind=0 filebench/filebench -f filebench/workloads/$TEST.f > ${filename}/Run$i
        sudo umount $PM_DEVICE
        if [ $FS = "squirrelfs" ]; then 
            sudo rmmod squirrelfs
        elif [ $FS = "nova" ]; then 
            sudo rmmod nova 
        elif [ $FS = "winefs" ]; then
            sudo rmmod winefs
        fi
    fi
done

