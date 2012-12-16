fuse-io-tracer
==============
This FUSE-based tracer records IO operations 
including read, write, open, close. The format 
of the output is like:

filepath operation offset length time

For example,
/tmp/myfile read 1024 3 23472342.2342842

or 

/tmp/myfile open NA NA 28423477752.3234243

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
