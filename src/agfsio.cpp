#include "agfsio.hpp"
#include <endian.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

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

int agfs_write_error(int fd, agerr_t err)
{
	err = htobe64(err);
	return write(fd, &err, sizeof(agerr_t));
}

int agfs_read_error(int fd, agerr_t& err)
{
	int error = read(fd, &err, sizeof(agerr_t));
	err = be64toh(err);
	return error;
}

int agfs_write_string(int fd, const char* str)
{
	int total_written = 0, err;
	agsize_t pathLen = htobe64(strlen(str));
	if ((err = write(fd, &pathLen, sizeof(agsize_t))) < 0)
	{
		return err;
	}
	total_written += err;

	if ((err = write(fd, str, pathLen)) < 0)
	{
		return err;
	}
	total_written += err;

	return total_written;
}

int agfs_read_string(int fd, std::string& str)
{
	int total_read = 0, err;
	agsize_t pathLen = 0;
	if ((err = read(fd, &pathLen, sizeof(agsize_t))) < 0)
	{
		return err;
	}
	total_read += err;
	pathLen = be64toh(pathLen);

	char* string = (char*)malloc(sizeof(char) * (pathLen + 1));
	err = read(fd, string, pathLen);
	string[pathLen] = 0;
	str = string;
	free(string);

	if (err < 0)
	{
		return err;
	}
	total_read += err;

	return total_read;
}

int agfs_write_stat(int fd, struct stat& buf)
{
	int temp, total_written = 0;

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
	int temp, total_read = 0;
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
