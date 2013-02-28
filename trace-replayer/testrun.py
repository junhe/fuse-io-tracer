#!/usr/bin/python

import itertools,subprocess
import os,sys
import subprocess # just to call an arbitrary command e.g. 'ls'
import time
import shlex

class cd:
    def __init__(self, newPath):
        self.newPath = newPath

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)



exefiles = ["./patfuse.linuxreadahead.e06d78474c", "./patfuse.random.06954905"]
parameters = [exefiles]
paralist = list(itertools.product(*parameters))



# Init the environment

jobid = time.strftime("%Y-%m-%d-%H-%M-%S", time.gmtime())

resultfilename = "results/" + jobid + ".result"
resultfile = open(resultfilename, "w")
resultfile.write("          Trace.Path           Data.Path     Sleep.Time      Customize.Sleeptime  Do.Pread    Bytes.Pread      Read.Time    Do.Prefetch Do.Period    Period Marker PatfuseType PreadTime PrefetchTime FadviseTime\n")

logfilename = "logs/" + jobid + ".log"
logfile = open(logfilename, "w")

wasteland = open("/dev/null", "w")

fpath = "/home/manio/workdir/pat-fuse/codeworkdir/pat-fuse/"
tracefile = "rrz009.localdomain1359504052.748747.trace"
for repid in range(5):
    for para in paralist:
        resultline = ""
        exefile = para[0]
        print(exefile)
        with cd(fpath):
            subprocess.call("pkill patfuse".split())
            time.sleep(3)
            subprocess.call("fusermount -u /mnt/fuse.1".split())
            subprocess.call("free -m".split())
            subprocess.call(shlex.split("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"))
            subprocess.call("free -m".split())

            fusecmd = [exefile, "/mnt/fuse.1", "-d", "-o", "direct_io"]
            subp_patfuse = subprocess.Popen(fusecmd, stdout=subprocess.PIPE,
                                      stderr=wasteland)
            time.sleep(3) 
            subprocess.call("ls /mnt/fuse.1".split())

        # run replayer
        replaycmd = ("./replayer " + tracefile +
                      " /mnt/fuse.1/tmp/bigfile 0 0 1 0 0 0").split()

        subp_replayer = subprocess.Popen(replaycmd, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)

        # pick results from replayer output
        for line in subp_replayer.stdout:
            if "GREPMARKER" in line:
                resultline = line.rstrip("\n")
            else:
                print line,
                sys.stdout.flush()
        
        subp_replayer.wait()

        subprocess.call("pkill patfuse".split())
        subp_patfuse.wait()

        # pick results from fuse output
        preadtime = "NA"
        prefetchtime = "NA"
        fadvisetime = "NA"

        for line in subp_patfuse.stdout:
            if "Profile" in line:
                if "pread" in line.lower():
                    preadtime = line.split()[-1]
                elif "prefetch" in line.lower():
                    prefetchtime = line.split()[-1]
                elif "fadvise" in line.lower():
                    fadvisetime = line.split()[-1]

        resultline += " " + exefile + " " + preadtime + " " + prefetchtime + " " + fadvisetime

        resultfile.write(resultline + "\n")

