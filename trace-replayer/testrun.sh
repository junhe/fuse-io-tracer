#!/bin/bash

#periodlist="32 100 320 640 1000 1600 3200 8000 10000 14000"
periodlist="1"
#periodlist="160 320 640 1000 1600 3200 6400 8000 10000"
#periodlist="11000 12000 13000 14000 20000"
#periodlist="14000"
fpath=/home/manio/workdir/pat-fuse/codeworkdir/pat-fuse/
mydir=`pwd`

for period in $periodlist
do
    for i in 1 2 3
    do
        #for exefile in patfuse.prefetch patfuse.noprefetch
        #for exefile in patfuse.noadvise
        #for exefile in patfuse.nopread
        #for exefile in patfuse.nopreadnoprefetch
        #for exefile in patfuse.prefetch
        #for exefile in patfuse.stupid
        #for exefile in patfuse.stupidwiththread
        #for exefile in patfuse.readahead
        #for exefile in patfuse.simple
        #for exefile in patfuse.simplereadahead
        #for exefile in patfuse.poolsimplereadahead patfuse.noprefetch
        #for exefile in patfuse.poolRANDOM-noprefetch patfuse.poolRANDOM-have-prefetch
        #for exefile in patfuse.poolRANDOM-have-prefetch0.9 patfuse.poolRANDOM-noprefetch-dump-before-read
        #for exefile in patfuse.RANDOM-prefetch patfuse.RANDOM-noprefetch
        #for exefile in patfuse.RANDOM-prefetch
        #for exefile in patfuse.pattern
        #for exefile in patfuse.savemarkov
        #for exefile in patfuse.real8192
        #for exefile in patfuse.perfectprefetch
        #for exefile in patfuse.vanilla
        for exefile in patfuse.adviseSEQ
        do
            echo ===============================================================
            export PREDICTPERIOD=$period
            echo -- period: $period --
            echo -- rep id: $i --

            cd $fpath
            pkill patfuse
            sleep 5
            fusermount -u /mnt/fuse.1
            free -m ; sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' ; free -m
            
            pwd
            ./$exefile /mnt/fuse.1 -d &> log  &
            sleep 5

            cd $mydir
            procinfo -b | grep sda
            cat /proc/diskstats |grep sda1
            echo $period $i $exefile `./replayer pagoda.trace /mnt/fuse.1/tmp/bigfile 0 0 1 0 0 0`
            
            pkill patfuse
            sleep 5
            cd $fpath
            tail log|grep Profile
            #grep PREFETCHING log
            cd $mydir
        done
    done
    procinfo -b | grep sda
    cat /proc/diskstats |grep sda1
done

