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

The time is got by gettimeofday(). When the data file
is opened, the trace file is opened at the same time.
The trace file is located where the FUSE program is run.
When the data file is closed, the trace file is closed.
The trace file is named the same as time-created.log.
