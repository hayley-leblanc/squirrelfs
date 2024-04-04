#!/bin/bash

gcc io_size_test.c -o io_size_test

echo "io size,file_system,avg write latency(us),avg read latency(us),write throughput(kb/ms),read throughput(kb/ms)" > io_size_output.csv

run_rust() {
    echo -n "rust,$1," >> io_size_output.csv
    sudo insmod ~/linux/fs/hayleyfs/hayleyfs.ko 
    sudo mount -t hayleyfs -o init /dev/pmem0 /mnt/pmem
    sudo numactl --membind 0 ./io_size_test $1 >> io_size_output.csv
    sudo umount /dev/pmem0 
    sudo rmmod hayleyfs 
}

run_ext4() {
    echo -n "ext4$2,$1," >> io_size_output.csv
    echo "yes" | sudo mkfs.ext4 /dev/pmem0 
    sudo numactl --membind 0 mount -t ext4 -o dax,data=$2 /dev/pmem0 /mnt/pmem/ 
    sudo ./io_size_test $1 >> io_size_output.csv
    sudo umount /dev/pmem0 
}

run_rust $((1024*4))
run_rust $((1024*8))
run_rust $((1024*16))
run_rust $((1024*64))
run_rust $((1024*256))
run_rust $((1024*512))
run_rust $((1024*1024))
run_rust $((1024*1024*2))

run_ext4 $((1024*4)) ordered
run_ext4 $((1024*8)) ordered
run_ext4 $((1024*16)) ordered
run_ext4 $((1024*64)) ordered
run_ext4 $((1024*256)) ordered
run_ext4 $((1024*512)) ordered
run_ext4 $((1024*1024)) ordered
run_ext4 $((1024*1024*2)) ordered

run_ext4 $((1024*4)) writeback
run_ext4 $((1024*8)) writeback
run_ext4 $((1024*16)) writeback
run_ext4 $((1024*64)) writeback
run_ext4 $((1024*256)) writeback
run_ext4 $((1024*512)) writeback
run_ext4 $((1024*1024)) writeback
run_ext4 $((1024*1024*2)) writeback
