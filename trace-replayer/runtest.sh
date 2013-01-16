#!/bin/bash


#./replayer trace-file output-file sleeptime customized-sleeptime do-pread
#./replayer pagoda.trace /mnt/fuse.1/tmp/bigfile 10 1 1

tracefile=pagoda.trace
outputfile=/mnt/fuse.1/tmp/bigfile
sleeptimelist="0 100 500 1000 5000 50000"
customizedlist="1"
dopreadlist="0 1"
resultfile="result.txt"

for sleeptime in $sleeptimelist
do
    for customizedsleeptime in $customizedlist
    do
        for dopread in $dopreadlist
        do
            sync
            sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
            echo $sleeptime $customizedsleeptime $dopread
            echo FIRST-RUN `./replayer $tracefile $outputfile $sleeptime $customizedsleeptime $dopread` >> $resultfile
            echo SECOND-RUN `./replayer $tracefile $outputfile $sleeptime $customizedsleeptime $dopread` >> $resultfile
        done
    done
done
