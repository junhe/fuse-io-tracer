fuse-io-tracer
==============
This FUSE-based tracer records IO operations 
including read, write, open, close. This tracer is very nice
because, no matter what I/O interfaces you use, it is able to 
record the traces, as long as you do your operation in that
mount point. The format of the output trace is like:

pid filepath operation offset length start-time end-time

For example,
8848 /tmp/myfile read 1024 3 23472342.2342842 332324243.23223

or 

8848 /tmp/myfile open NA NA 28423477752.3234243 23242444222324.3232

The time is got by gettimeofday(). When the FUSE program
is run, the trace file is opened at the same time.
The trace file is located where the FUSE program is run.
When the FUSE program is closed, the trace file is closed.
The trace file is named the same as time-created.log.


To run:
./fusetracer /mnt/trace -d -o direct_io

You have to use direct io, otherwise FUSE does its
own caching and breaks the pattern with 4k aligned
r/w.

To use:
./myapplication /mnt/trace/home/junhe/bigfile

If your application accesses multiple files and you only want
the trace of a subset of the files, you can do this:
./myapplication /mnt/trace/home/junhe/file-to-be-traced /home/junhe/file-no-to-be-traced
