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
	int temp, total_written;

	agdev_t dev = buf.st_dev;
	if ((temp = write(fd, &dev, sizeof(agdev_t))) < 0) {
		return temp;
	}
	total_written += temp;

	agmode_t mode = buf.st_mode;
	if ((temp = write(fd, &mode, sizeof(agmode_t))) < 0) {
		return temp;
	}
	total_written += temp;

	agsize_t size = buf.st_size;
	if ((temp = write(fd, &size, sizeof(agsize_t))) < 0) {
		return temp;
	}
	total_written += temp;

	return total_written;
}

int agfs_read_stat(int fd, struct stat& buf)
{
	int temp, total_read;
	memset(&buf, 0, sizeof(struct stat));

	agdev_t dev;
	if ((temp = write(fd, &dev, sizeof(agdev_t))) < 0) {
		return temp;
	}
	total_read += temp;

	agmode_t mode;
	if ((temp = read(fd, &mode, sizeof(agmode_t))) < 0) {
		return temp;
	}
	total_read += temp;

	agsize_t size;
	if ((temp = write(fd, &size, sizeof(agsize_t))) < 0) {
		return temp;
	}
	total_read += temp;

	buf.st_size = size;
	buf.st_mode = mode;
	buf.st_dev = dev;

	return total_read;
}
