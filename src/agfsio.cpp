#include "agfsio.hpp"
#include <endian.h>
#include <unistd.h>

int agfs_write_cmd(int fd, cmd_t cmd)
{
	cmd = htobe16(cmd);
	return write(fd, &cmd, sizeof(cmd_t));
}

int agfs_read_cmd(int fd, cmd_t& cmd)
{
	int err = read(fd, &cmd, sizeof(cmd_t));
	cmd = be16toh(cmd);
	return err;
}

struct agfs_stat {
	agdev_t st_dev;
	agmode_t st_mode;
	agsize_t st_size;
};

int agfs_write_stat(int fd, struct stat& buf)
{
	struct agfs_stat result;
	memset(&result, 0, sizeof(struct agfs_stat));
	result.st_dev = buf.st_dev;
	result.st_mode = buf.st_mode;
	result.size = buf.st_size;

	return write(fd, &result, sizeof(struct stat));
}

int agfs_read_stat(int fd, struct stat& buf)
{
	struct agfs_stat result;
	memset(&result, 0, sizeof(struct agfs_stat));

	//Switch the bytes of all the fields around.
	int err = 0;
	if ((err = read(fd, &result, sizeof(struct agfs_stat))) >= 0) {
		memset(&buf, 0, sizeof(struct stat));
		buf.st_dev = be64toh(result.st_dev);
		buf.st_mode = be32toh(result.st_mode);
		buf.st_size = be64toh(result.st_size);
	}

	return err;
}
