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
