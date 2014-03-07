/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

static int agfs_getattr(const char *path, struct stat *stbuf)
{
  int res;

  res = lstat(path, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_access(const char *path, int mask)
{
  int res;

  res = access(path, mask);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_readlink(const char *path, char *buf, size_t size)
{
  int res;

  res = readlink(path, buf, size - 1);
  if (res == -1)
    return -errno;

  buf[res] = '\0';
  return 0;
}


static int agfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
  DIR *dp;
  struct dirent *de;

  (void) offset;
  (void) fi;

  dp = opendir(path);
  if (dp == NULL)
    return -errno;

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (filler(buf, de->d_name, &st, 0))
      break;
  }

  closedir(dp);
  return 0;
}

static int agfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  int res;

  /* On Linux this could just be 'mknod(path, mode, rdev)' but this
     is more portable */
  if (S_ISREG(mode)) {
    res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0)
      res = close(res);
  } else if (S_ISFIFO(mode))
    res = mkfifo(path, mode);
  else
    res = mknod(path, mode, rdev);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_mkdir(const char *path, mode_t mode)
{
  int res;

  res = mkdir(path, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_unlink(const char *path)
{
  int res;

  res = unlink(path);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_rmdir(const char *path)
{
  int res;

  res = rmdir(path);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_symlink(const char *to, const char *from)
{
  int res;

  res = symlink(to, from);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_rename(const char *from, const char *to)
{
  int res;

  res = rename(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_link(const char *from, const char *to)
{
  int res;

  res = link(from, to);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_chmod(const char *path, mode_t mode)
{
  int res;

  res = chmod(path, mode);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int res;

  res = lchown(path, uid, gid);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_truncate(const char *path, off_t size)
{
  int res;

  res = truncate(path, size);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_utimens(const char *path, const struct timespec ts[2])
{
  int res;
  struct timeval tv[2];

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;

  res = utimes(path, tv);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_open(const char *path, struct fuse_file_info *fi)
{
  int res;

  res = open(path, fi->flags);
  if (res == -1)
    return -errno;

  close(res);
  return 0;
}

static int agfs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
  int fd;
  int res;

  (void) fi;
  fd = open(path, O_RDONLY);
  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);
  return res;
}

static int agfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
  int fd;
  int res;

  (void) fi;
  fd = open(path, O_WRONLY);
  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);
  return res;
}

static int agfs_statfs(const char *path, struct statvfs *stbuf)
{
  int res;

  res = statvfs(path, stbuf);
  if (res == -1)
    return -errno;

  return 0;
}

static int agfs_release(const char *path, struct fuse_file_info *fi)
{
  /* Just a stub. This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) fi;
  return 0;
}

static int agfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
  /* Just a stub. This method is optional and can safely be left
     unimplemented */

  (void) path;
  (void) isdatasync;
  (void) fi;
  return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int agfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
  int res = lsetxattr(path, name, value, size, flags);
  if (res == -1)
    return -errno;
  return 0;
}

static int agfs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
  int res = lgetxattr(path, name, value, size);
  if (res == -1)
    return -errno;
  return res;
}

static int agfs_listxattr(const char *path, char *list, size_t size)
{
  int res = llistxattr(path, list, size);
  if (res == -1)
    return -errno;
  return res;
}

static int agfs_removexattr(const char *path, const char *name)
{
  int res = lremovexattr(path, name);
  if (res == -1)
    return -errno;
  return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations agfs_oper = {
  .getattr= agfs_getattr,
  .access= agfs_access,
  .readlink= agfs_readlink,
  .readdir= agfs_readdir,
  .mknod= agfs_mknod,
  .mkdir= agfs_mkdir,
  .symlink= agfs_symlink,
  .unlink= agfs_unlink,
  .rmdir= agfs_rmdir,
  .rename= agfs_rename,
  .link= agfs_link,
  .chmod= agfs_chmod,
  .chown= agfs_chown,
  .truncate= agfs_truncate,
  .utimens= agfs_utimens,
  .open= agfs_open,
  .read= agfs_read,
  .write= agfs_write,
  .statfs= agfs_statfs,
  .release= agfs_release,
  .fsync= agfs_fsync,
#ifdef HAVE_SETXATTR
  .setxattr= agfs_setxattr,
  .getxattr= agfs_getxattr,
  .listxattr= agfs_listxattr,
  .removexattr= agfs_removexattr,
#endif
};

int main(int argc, char *argv[])
{
  umask(0);
  return fuse_main(argc, argv, &agfs_oper, NULL);
}