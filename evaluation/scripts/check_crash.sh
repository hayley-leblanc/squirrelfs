#!/bin/bash

read_dir () {
    ls_output=$(ls $1)
    for f in $ls_output
    do 
        stat $1/$f > /dev/null
        if [[ -d $1/$f ]]
        then 
            read_dir $1/$f
        else 
            cat $1/$f > /dev/null
        fi 
    done

    if [ $1 = "/mnt/pmem/src_parent" ]
    then 
        for f in $ls_output
        do 
            if [ -d /mnt/pmem/dst_parent/$f ]
            then 
                echo "error: $f is in dst and src"
            fi
        done
    fi 

    touch $1/dummy > /dev/null
    mkdir $1/dummy_dir > /dev/null
    rm $1/dummy > /dev/null
    rmdir $1/dummy_dir > /dev/null
}

# sudo dd if=../snapshot of=/dev/pmem0

sudo -E insmod $HOME/linux/fs/hayleyfs/hayleyfs.ko
sudo mount -t hayleyfs /dev/pmem0 /mnt/pmem/ > /dev/null

stat /mnt/pmem
read_dir /mnt/pmem

sudo umount /dev/pmem0
sudo rmmod hayleyfs