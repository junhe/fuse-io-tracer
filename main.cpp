/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusetrc_fh.c `pkg-config fuse --cflags --libs` -lulockmgr -o fusetrc_fh
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#define _GNU_SOURCE

#include <fuse.h>
#include <ulockmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <sys/file.h> /* flock(2) */

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

FILE* trcfp = NULL; // fp for trace file. It has to be opened before everthing
               // else starts, and be closed after everything.
int opcnt = 0;

static int trc_getattr(const char *path, struct stat *stbuf)
{
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_fgetattr(const char *path, struct stat *stbuf,
			struct fuse_file_info *fi)
{
	int res;

	(void) path;

	res = fstat(fi->fh, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_access(const char *path, int mask)
{
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_readlink(const char *path, char *buf, size_t size)
{
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

struct trc_dirp {
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

static int trc_opendir(const char *path, struct fuse_file_info *fi)
{
	int res;
	struct trc_dirp *d = (struct trc_dirp *)malloc(sizeof(struct trc_dirp));
	if (d == NULL)
		return -ENOMEM;

	d->dp = opendir(path);
	if (d->dp == NULL) {
		res = -errno;
		free(d);
		return res;
	}
	d->offset = 0;
	d->entry = NULL;

	fi->fh = (unsigned long) d;
	return 0;
}

static inline struct trc_dirp *get_dirp(struct fuse_file_info *fi)
{
	return (struct trc_dirp *) (uintptr_t) fi->fh;
}

static int trc_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	struct trc_dirp *d = get_dirp(fi);

	(void) path;
	if (offset != d->offset) {
		seekdir(d->dp, offset);
		d->entry = NULL;
		d->offset = offset;
	}
	while (1) {
		struct stat st;
		off_t nextoff;

		if (!d->entry) {
			d->entry = readdir(d->dp);
			if (!d->entry)
				break;
		}

		memset(&st, 0, sizeof(st));
		st.st_ino = d->entry->d_ino;
		st.st_mode = d->entry->d_type << 12;
		nextoff = telldir(d->dp);
		if (filler(buf, d->entry->d_name, &st, nextoff))
			break;

		d->entry = NULL;
		d->offset = nextoff;
	}

	return 0;
}

static int trc_releasedir(const char *path, struct fuse_file_info *fi)
{
	struct trc_dirp *d = get_dirp(fi);
	(void) path;
	closedir(d->dp);
	free(d);
	return 0;
}

static int trc_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_chmod(const char *path, mode_t mode)
{
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_truncate(const char *path, off_t size)
{
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_ftruncate(const char *path, off_t size,
			 struct fuse_file_info *fi)
{
	int res;

	(void) path;

	res = ftruncate(fi->fh, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int trc_utimens(const char *path, const struct timespec ts[2])
{
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int trc_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fd;

	fd = open(path, fi->flags, mode);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int trc_open(const char *path, struct fuse_file_info *fi)
{
	int fd;
    struct timeval stime, etime; // start time, end time

    gettimeofday(&stime, NULL);
	fd = open(path, fi->flags);
    gettimeofday(&etime, NULL);

    // Trace
    fprintf(trcfp, "%d %s %s NA NA %ld.%.6ld %ld.%.6ld\n",
            fuse_get_context()->pid, path, __FUNCTION__, 
            stime.tv_sec, stime.tv_usec,
            etime.tv_sec, etime.tv_usec);
    if ( opcnt++ % 100 == 0 ) fflush(trcfp);

	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int trc_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int res;

	(void) path;
    struct timeval stime, etime; // start time, end time

    gettimeofday(&stime, NULL);
    
    // operation
    res = pread(fi->fh, buf, size, offset);
    
    gettimeofday(&etime, NULL);

    // Trace
    fprintf(trcfp, "%d %s %s %lld %u %f %f\n",
            fuse_get_context()->pid, path, __FUNCTION__, 
            offset, size,
            stime.tv_sec + stime.tv_usec/1000000.0,
            etime.tv_sec + etime.tv_usec/1000000.0);
    if ( opcnt++ % 100 == 0 ) fflush(trcfp);

	if (res == -1)
		res = -errno;

	return res;
}

#if 0
static int trc_read_buf(const char *path, struct fuse_bufvec **bufp,
			size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct fuse_bufvec *src;

	(void) path;

    struct timeval stime, etime; // start time, end time

    gettimeofday(&stime, NULL);
    
    // operation
	src = (struct fuse_bufvec *) malloc(sizeof(struct fuse_bufvec));
	if (src == NULL)
		return -ENOMEM;

	*src = FUSE_BUFVEC_INIT(size);

	src->buf[0].flags = (fuse_buf_flags)(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
	src->buf[0].fd = fi->fh;
	src->buf[0].pos = offset;

	*bufp = src;
    //////////////////////
  

    gettimeofday(&etime, NULL);

    // Trace
    fprintf(trcfp, "%d %s %s %lld %u %ld.%.6ld %ld.%.6ld\n",
            fuse_get_context()->pid, path, __FUNCTION__, 
            offset, size,
            stime.tv_sec, stime.tv_usec,
            etime.tv_sec, etime.tv_usec);
    if ( opcnt++ % 100 == 0 ) fflush(trcfp);

	return 0;
}
#endif

static int trc_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int res;

	(void) path;

    struct timeval stime, etime; // start time, end time

    gettimeofday(&stime, NULL);

	res = pwrite(fi->fh, buf, size, offset);
	
    gettimeofday(&etime, NULL);

    // Trace
    fprintf(trcfp, "%d %s %s %lld %u %ld.%.6ld %ld.%.6ld\n",
            fuse_get_context()->pid, path, __FUNCTION__, 
            offset, size,
            stime.tv_sec, stime.tv_usec,
            etime.tv_sec, etime.tv_usec);
    if ( opcnt++ % 100 == 0 ) fflush(trcfp);
    

    if (res == -1)
		res = -errno;

	return res;
}

#if 0
static int trc_write_buf(const char *path, struct fuse_bufvec *buf,
		     off_t offset, struct fuse_file_info *fi)
{
	struct fuse_bufvec dst = FUSE_BUFVEC_INIT(fuse_buf_size(buf));

	(void) path;

    struct timeval stime, etime; // start time, end time

    gettimeofday(&stime, NULL);


	dst.buf[0].flags = (fuse_buf_flags)(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
	dst.buf[0].fd = fi->fh;
	dst.buf[0].pos = offset;

	int ret = fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK);
    
    gettimeofday(&etime, NULL);

    // Trace
    fprintf(trcfp, "%d %s %s %lld %u %ld.%.6ld %ld.%.6ld\n",
            fuse_get_context()->pid, path, __FUNCTION__, 
            offset, fuse_buf_size(buf),
            stime.tv_sec, stime.tv_usec,
            etime.tv_sec, etime.tv_usec);
    if ( opcnt++ % 100 == 0 ) fflush(trcfp);

    return ret;
}
#endif

static int trc_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int trc_flush(const char *path, struct fuse_file_info *fi)
{
	int res;

	(void) path;
	
    struct timeval stime, etime; // start time, end time

    gettimeofday(&stime, NULL);

    /* This is called from every close on an open file, so call the
	   close on the underlying filesystem.	But since flush may be
	   called multiple times for an open file, this must not really
	   close the file.  This is important if used on a network
	   filesystem like NFS which flush the data/metadata on close() */
	res = close(dup(fi->fh));
	if (res == -1)
		return -errno;

    gettimeofday(&etime, NULL);

    // Trace
    fprintf(trcfp, "%d %s %s NA NA %ld.%.6ld %ld.%.6ld\n",
            fuse_get_context()->pid, path, __FUNCTION__, 
            stime.tv_sec, stime.tv_usec,
            etime.tv_sec, etime.tv_usec);
    if ( opcnt++ % 100 == 0 ) fflush(trcfp);

	return 0;
}

static int trc_release(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	close(fi->fh);

	return 0;
}

static int trc_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	int res;
	(void) path;

#ifndef HAVE_FDATASYNC
	(void) isdatasync;
#else
	if (isdatasync)
		res = fdatasync(fi->fh);
	else
#endif
		res = fsync(fi->fh);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int trc_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	(void) path;

	if (mode)
		return -EOPNOTSUPP;

	return -posix_fallocate(fi->fh, offset, length);
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int trc_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int trc_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int trc_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int trc_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static int trc_lock(const char *path, struct fuse_file_info *fi, int cmd,
		    struct flock *lock)
{
	(void) path;

//    ret =  ulockmgr_op(fi->fh, cmd, lock, &fi->lock_owner,
//			   sizeof(fi->lock_owner));
    return EXIT_FAILURE;
}

static int trc_flock(const char *path, struct fuse_file_info *fi, int op)
{
	int res;
	(void) path;

	res = flock(fi->fh, op);
	if (res == -1)
		return -errno;

	return 0;
}

static struct fuse_operations trc_oper;
void load_operations()
{
	trc_oper.getattr	= trc_getattr;
	trc_oper.fgetattr	= trc_fgetattr;
	trc_oper.access		= trc_access;
	trc_oper.readlink	= trc_readlink;
	trc_oper.opendir	= trc_opendir;
	trc_oper.readdir	= trc_readdir;
	trc_oper.releasedir	= trc_releasedir;
	trc_oper.mknod		= trc_mknod;
	trc_oper.mkdir		= trc_mkdir;
	trc_oper.symlink	= trc_symlink;
	trc_oper.unlink		= trc_unlink;
	trc_oper.rmdir		= trc_rmdir;
	trc_oper.rename		= trc_rename;
	trc_oper.link		= trc_link;
	trc_oper.chmod		= trc_chmod;
	trc_oper.chown		= trc_chown;
	trc_oper.truncate	= trc_truncate;
	trc_oper.ftruncate	= trc_ftruncate;
#ifdef HAVE_UTIMENSAT
	trc_oper.utimens	= trc_utimens;
#endif
	trc_oper.create		= trc_create;
	trc_oper.open		= trc_open;
	trc_oper.read		= trc_read;
	//trc_oper.read_buf	= trc_read_buf;
	trc_oper.write		= trc_write;
	//trc_oper.write_buf	= trc_write_buf;
	trc_oper.statfs		= trc_statfs;
	trc_oper.flush		= trc_flush;
	trc_oper.release	= trc_release;
	trc_oper.fsync		= trc_fsync;
#ifdef HAVE_POSIX_FALLOCATE
	trc_oper.fallocate	= trc_fallocate;
#endif
#ifdef HAVE_SETXATTR
	trc_oper.setxattr	= trc_setxattr;
	trc_oper.getxattr	= trc_getxattr;
	trc_oper.listxattr	= trc_listxattr;
	trc_oper.removexattr	= trc_removexattr;
#endif
	trc_oper.lock		= trc_lock;
	trc_oper.flock		= trc_flock;

	trc_oper.flag_nullpath_ok = 1;
#if HAVE_UTIMENSAT
	trc_oper.flag_utime_omit_ok = 1;
#endif
}

int main(int argc, char *argv[])
{
    struct timeval create_time;
    ostringstream trc_file_name;
    int ret;
    
    gettimeofday(&create_time, NULL);
    trc_file_name << setfill('0');
    trc_file_name << create_time.tv_sec 
                  << "."
                  << setw(6)
                  << create_time.tv_usec << ".trace";   


    trcfp = fopen(trc_file_name.str().c_str(), "w"); 
    if ( trcfp == NULL ) {
        perror("fopen");
        return -1;
    }
    
	umask(0);
	load_operations();
	ret = fuse_main(argc, argv, &trc_oper, NULL);
    
    fclose(trcfp);
    return ret;
}

