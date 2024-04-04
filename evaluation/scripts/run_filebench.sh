#!/bin/bash
TEST=$1
FS=$2

iterations=5
filename=output-temp/${FS}/filebench/${TEST}
mkdir -p $filename

echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
for i in $(seq $iterations)
do
    if [ $FS = "squirrelfs" ]; then 
        sudo -E insmod $HOME/linux/fs/hayleyfs/hayleyfs.ko; sudo mount -t hayleyfs -o init /dev/pmem0 /mnt/pmem/
    elif [ $FS = "nova" ]; then 
        sudo -E insmod $HOME/linux/fs/nova/nova.ko; sudo mount -t NOVA -o init /dev/pmem0 /mnt/pmem/
    elif [ $FS = "winefs" ]; then 
        sudo -E insmod $HOME/linux/fs/winefs/winefs.ko; sudo mount -t winefs -o init /dev/pmem0 /mnt/pmem/
    elif [ $FS = "ext4" ]; then 
        yes | sudo mkfs.ext4 /dev/pmem0 
        sudo -E mount -t ext4 -o dax /dev/pmem0 /mnt/pmem/
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
        sudo umount /dev/pmem0
        if [ $FS = "squirrelfs" ]; then 
            sudo rmmod hayleyfs
        elif [ $FS = "nova" ]; then 
            sudo rmmod nova 
        elif [ $FS = "winefs" ]; then
            sudo rmmod winefs
        fi
    fi
done
