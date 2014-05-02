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


// Non-FUSE includes
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "serverconnection.hpp"
#include "disambiguater.hpp"
#include <sys/types.h>
#include <map>
#include <chrono>
#include <thread>

/**************
 * GLOBALS *
 ***********/


//Allocated onto the heap to allow for quick access to which server holds
//a specific file (cuts down on network traffic).
typedef struct {
  std::string server;
} file_handle_t;

static std::map<std::string, ServerConnection> connections;
static std::vector<std::thread> heartbeatThreads;

/*
 * Assumes an ambiguated path input and queries all servers
 * for that file.
 */
std::vector<std::string> checkExistance(const char* path) {
  std::vector<std::string> retVal{};

  std::map<std::string, ServerConnection>::iterator it;
  agerr_t error = 0;
  for (it = connections.begin(); it != connections.end(); ++it) {
    error = it->second.access(path, F_OK);
    if (error >= 0) {
      retVal.push_back(it->first);
    }
  }

  return retVal;
}

static void agfs_destroy(void *data)
{
  (void)data;
  
  //Loop through server connections and send stop signal.
  std::map<std::string, ServerConnection>::iterator it;
  std::vector<std::future<aggerr_t>> futures;
  for (it = connections.begin(); it != connections.end(); ++it) {
      futures.push(it->second.stop());
  }
  
  // Wait for all the connections to close;
  for(std::future<aggerr_t>& theFuture : futures) {
      theFuture.get();
  }
  
  for (size_t i = 0; i < heartbeatThreads.size(); i++) {
    heartbeatThreads[i].join();
  }
}

static int agfs_getattr(const char *path, struct stat *stbuf)
{
  memset(stbuf, 0, sizeof(struct stat));

  //Initialize useful structures
  std::pair<std::string, std::string> id{Disambiguater::ambiguate(path)};
  std::string server{id.first}, file{id.second};
  std::vector<std::future<std::pair<struct stat, agerr_t>>> futures;
  std::pair<struct stat, agerr_t> retVal;
  //If the server is in our map, then only look at that server
  std::map<std::string, ServerConnection>::iterator it;
  it = connections.find(server);
  if (it != connections.end()) {
      futures.push_back(it->second.getattr(file.c_str()));
      retVal = futures[0].get();
  } else {
    //Otherwise, iterate through all connections to find the first such file.
    for (it = connections.begin(); it != connections.end(); ++it) {
      if (it->second.connected()) {
          futures.push_back(it->second.getattr(file.c_str()));
      }
    }

    for(std::future<std::pair<struct stat, agerr_t>>& aFuture : futures) {
        retVal = aFuture;
        if (retVal.second >= 0) {
            break;
        }
    }  
  }
  

  agerr_t error = retVal.second;
  if (error >= 0) {
    (*stbuf) = retVal.first;
  }

  return error;
}

static int agfs_access(const char *path, int mask)
{
  agerr_t res = 0;

  //Initialize useful structures
  std::pair<std::string, std::string> id{Disambiguater::ambiguate(path)};
  std::string server{id.first}, file{id.second};
  std::vector<std::future<agerr_t>> futures;

  //Find the file
  std::map<std::string, ServerConnection>::iterator it;
  it = connections.find(server);
  if (it != connections.end()) {
      res = (it->second.access(file.c_str(), mask)).get();
  } else {
      for (it = connections.begin(); it != connections.end(); ++it) {
          futures.push_back(it->second.access(file.c_str(), mask));
          if (res >= 0) {
              break;
          }
      }
      
      for(std::future<agerr_t>> aFuture& : futures) {
          res = aFuture.get();
          if (res >= 0) {
              break;
          }
      }
  }
  
  return res;
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

static int agfs_opendir(const char* path, struct fuse_file_info *fi)
{
  // We might want to place a file handle in the fi struct for later use.
  (void)path;
  (void)fi;
  std::cerr << "OpenDir unimplemented" << std::endl;
  return 0;
}

static int agfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
  (void) fi;
  (void) offset;

  //We require two errors, one that is persistent, one that is temporary
  agerr_t err = -ENOTDIR, temp;

  //Initialize useful structures
  std::pair<std::string, std::string> id{Disambiguater::ambiguate(path)};
  std::string server{id.first}, file{id.second};
  std::vector<std::future<std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t>>> futures;
  Disambiguater disam{};

  std::map<std::string, ServerConnection>::iterator it = connections.find(server);
  if (it != connections.end()) {
      std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t> retVal{it->second.readdir(file.c_str()).get()};
      if ((temp = std::get<1>(retVal)) >= 0) {
          disam.addFilepaths(std::get<0>(retVal), it->first);
          err = temp;
      }
  } else {
      for (it = connections.begin(); it != connections.end(); ++it) {
          futures.push_back(it->second.readdir(file.c_str()));
      }
      
      for (std::future<std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t>>& aFuture : futures) {        
        std::pair<std::vector<std::pair<std::string, struct stat>>, agerr_t> retVal{aFuture.get()};
        if ((temp = std::get<1>(retVal)) >= 0) {
            disam.addFilepaths(std::get<0>(retVal), it->first);
            err = temp;
        }
      }
  }
  
  

  //Fill the buffer with the filenames that were found.
  std::vector<std::pair<std::string, struct stat>> disamPathsStats{disam.disambiguatedFilepathsWithStats()};
  for (size_t i = 0; i < disamPathsStats.size(); i++) {
    filler(buf, disamPathsStats[i].first.c_str(), &disamPathsStats[i].second, 0);
  }
  disam.clearPaths();

  return err;
}

static int agfs_releasedir(const char* path, struct fuse_file_info *fi)
{
  (void) path;
  (void) fi;
  std::cerr << "ReleaseDir unimplemented" << std::endl;
  // We should probably remove the file handle and any associated data.
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
  std::pair<std::string, std::string> id{Disambiguater::ambiguate(path)};
  std::string server{id.first}, file{id.second};

  if (server.length() == 0) {
    std::vector<std::string> servers = checkExistance(path);
    if (servers.size() != 1) {
      return -ENOENT;
    }
    server = servers[0];
  }

  std::map<std::string, ServerConnection>::iterator it;
  it = connections.find(server);
  agerr_t error = -ENOENT;
  if (it != connections.end()) {
    error = it->second.open(file.c_str(), fi->flags);
    if (error >= 0) {
      file_handle_t* fileHandle = new file_handle_t{};
      fileHandle->server = server;
      fi->fh = (uintptr_t)fileHandle;
    }
  }

  return error;
}

static int agfs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
  std::pair<std::string, std::string> id{Disambiguater::ambiguate(path)};
  std::string server{id.first}, file{id.second};

  if (server.length() == 0) {
    file_handle_t* fileHandle = (file_handle_t*)(fi->fh);
    server = fileHandle->server;
  }

  std::map<std::string, ServerConnection>::iterator it;
  std::pair<std::vector<unsigned char>, agerr_t> retVal;
  retVal.second = -ENOENT;
  it = connections.find(server);
  if (it != connections.end()) {
    retVal = it->second.readFile(file.c_str(), size, offset);
    if (retVal.second >= 0) {
      memcpy(buf, &retVal.first[0], retVal.first.size());
    }
  }

  return retVal.second >= 0 ? retVal.first.size() : retVal.second;
}

static int agfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
  std::pair<std::string, std::string> id{Disambiguater::ambiguate(path)};
  std::string server{id.first}, file{id.second};

  if (server.length() == 0) {
    file_handle_t* fileHandle = (file_handle_t*)(fi->fh);
    server = fileHandle->server;
  }

  std::map<std::string, ServerConnection>::iterator it;
  std::pair<agsize_t, agerr_t> retVal;
  retVal.second = -ENOENT;
  it = connections.find(server);
  if (it != connections.end()) {
    retVal = it->second.writeFile(file.c_str(), size, offset, buf);
  }

  return retVal.second >= 0 ? retVal.first : retVal.second;
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

  file_handle_t* fileHandle = (file_handle_t*)((uintptr_t)fi->fh);
  delete fileHandle;

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

/************************************
 * OPTIONS PROCESSING (Not working) *
 ************************************/


/*struct agfs_config {
  char* instanceFile;
};

enum {
     KEY_HELP,
     KEY_VERSION,
};
#define AGFS_OPT(t, p, v) { t, offsetof(struct agfs_config, p), v }

static struct fuse_opt agfs_opts[] = {
     AGFS_OPT("-i %s",          instanceFile, 0),
     AGFS_OPT("--instance=%s", instanceFile, 0),
     AGFS_OPT("instance=%s", instanceFile, 0),

     FUSE_OPT_KEY("-V",             KEY_VERSION),
     FUSE_OPT_KEY("--version",      KEY_VERSION),
     FUSE_OPT_KEY("-h",             KEY_HELP),
     FUSE_OPT_KEY("--help",         KEY_HELP),
     FUSE_OPT_END
};

#define PACKAGE_VERSION "0.0.0 beta"


static int agfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
     switch (key) {
     case KEY_HELP:
             fprintf(stderr,
                     "usage: %s mountpoint [options]\n"
                     "\n"
                     "general options:\n"
                     "    -o opt,[opt...]  mount options\n"
                     "    -h   --help      print help\n"
                     "    -V   --version   print version\n"
                     "\n"
                     "agfs options:\n"
                     "    -o instance=STRING\n"
                     "    -i STRING          same as '-o instance=STRING'\n"
                     "    --instance=STRING    same as -i STRING'\n\n"
                     , outargs->argv[0]);
             fuse_opt_add_arg(outargs, "-ho");
             fuse_main(outargs->argc, outargs->argv, &agfs_oper, NULL);
             exit(1);

     case KEY_VERSION:
             fprintf(stderr, "agfs version %s\n", PACKAGE_VERSION);
             fuse_opt_add_arg(outargs, "--version");
             fuse_main(outargs->argc, outargs->argv, &agfs_oper, NULL);
             exit(0);
     }
     return 1;
}*/


void heartbeatThread(ServerConnection& conn) {
  std::chrono::seconds beatTime{5};
  std::chrono::seconds reconnTime{30};
  while(!conn.closed()) {
    if (!conn.connected()) {
      std::this_thread::sleep_for(reconnTime);
      conn.connect();
    } else {
      std::this_thread::sleep_for(beatTime);
      conn.heartbeat();
    }
  }
}

static const boost::filesystem::path KEYDIRPATH(".agfs");
static const std::string EXTENSION(".agkey");

using namespace boost::filesystem;

static struct fuse_operations agfs_oper;

int main(int argc, char *argv[])
{
  path homeDir{getenv("HOME")};
  homeDir /= KEYDIRPATH;
  if(!exists(homeDir)) {
    std::cerr << "Could not find key directory: ~/.agfs" << std::endl;
    exit(1);
  }

  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( homeDir ); itr != end_itr; ++itr ) {
    if( is_regular_file(*itr) ) {
      if( extension(itr->path()) == EXTENSION) {
        std::cout << "Found keyfile: " << itr->path() << std::endl;
        std::fstream keyfile;
        keyfile.open(itr->path().native(), std::fstream::in);
        if(keyfile.is_open()) {
          std::cout << "Opened keyfile" << std::endl;
          std::string hostname, port, key;
          keyfile >> hostname;
          keyfile >> port;
          keyfile >> key;
          ServerConnection connection{hostname, port, key};
          if (connection.connected() && !connection.stopped()) {
            connections.insert(std::make_pair(hostname, connection));
          }
        }
        keyfile.close();
      }
    }
  }

  for (auto& connPair: connections) {
    heartbeatThreads.push_back(std::thread(heartbeatThread, std::ref(connPair.second)));
  }

  memset(&agfs_oper, 0, sizeof(struct fuse_operations));

  agfs_oper.destroy = agfs_destroy;
  agfs_oper.getattr = agfs_getattr;
  agfs_oper.access = agfs_access;
  agfs_oper.readlink = agfs_readlink;
  agfs_oper.opendir = agfs_opendir;
  agfs_oper.readdir = agfs_readdir;
  agfs_oper.releasedir = agfs_releasedir;
  agfs_oper.mknod = agfs_mknod;
  agfs_oper.mkdir = agfs_mkdir;
  agfs_oper.symlink = agfs_symlink;
  agfs_oper.unlink = agfs_unlink;
  agfs_oper.rmdir = agfs_rmdir;
  agfs_oper.rename = agfs_rename;
  agfs_oper.link = agfs_link;
  agfs_oper.chmod = agfs_chmod;
  agfs_oper.chown = agfs_chown;
  agfs_oper.truncate = agfs_truncate;
  agfs_oper.utimens = agfs_utimens;
  agfs_oper.open = agfs_open;
  agfs_oper.read = agfs_read;
  agfs_oper.write = agfs_write;
  agfs_oper.statfs = agfs_statfs;
  agfs_oper.release = agfs_release;
  agfs_oper.fsync = agfs_fsync;
#ifdef HAVE_SETXATTR
  agfs_oper.setxattr = agfs_setxattr;
  agfs_oper.getxattr = agfs_getxattr;
  agfs_oper.listxattr = agfs_listxattr;
  agfs_oper.removexattr = agfs_removexattr;
#endif

  return fuse_main(argc, argv, &agfs_oper, NULL);
}
