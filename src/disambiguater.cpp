
#include "disambiguater.hpp"
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

const std::string Disambiguater::BEGIN_BRACE = "{:";
const std::string Disambiguater::END_BRACE = ":}";

Disambiguater::Disambiguater()
	:filemap_{},
	 directorymap_{}
{
	//Nothing to do here...
}

agerr_t Disambiguater::addFilepath(std::pair<std::string, struct stat> value, std::string server)
{
	std::string path{value.first};
	struct stat stbuf = value.second;
	if (S_ISDIR(stbuf.st_mode)) {
		directorymap_[path] = stbuf;
	} else {
		for (size_t i = 0; i < filemap_[path].size(); i++) {
			if (filemap_[path][i].first == server) {
				return -EEXIST;
			}
		}
		filemap_[path].push_back(std::pair<std::string, struct stat>{server, stbuf});
	}

	return 0;
}

agerr_t Disambiguater::addFilepath(std::string path, std::string server)
{
	struct stat stbuf;
	memset(&stbuf, 0, sizeof(struct stat));
	//We don't need to check if the map has the path in it, since
	//the map will just create a new vector if it doesn't contain 
	//the path.
	std::pair<std::string, struct stat> value{server, stbuf};
	for (size_t i = 0; i < filemap_[path].size(); i++) {
		if (filemap_[path][i].first == server) {
			return -EEXIST;
		}
	}

	filemap_[path].push_back(value);
	return 0;
}

agerr_t Disambiguater::addFilepaths(std::vector<std::pair<std::string, struct stat>> paths, std::string server)
{
	agerr_t error = 0;
	for (size_t i = 0; i < paths.size(); i++) {
      if ((error = addFilepath(paths[i], server)) < 0) {
      	break;
      }
    }
    return error;
}

agerr_t Disambiguater::addFilepaths(std::vector<std::string> paths, std::string server)
{
	agerr_t error = 0;
	for (size_t i = 0; i < paths.size(); i++) {
      if ((error = addFilepath(paths[i], server)) < 0) {
      	break;
      }
    }
    return error;
}

std::vector<std::string> Disambiguater::disambiguatedFilepaths()
{
	std::vector<std::string> filenames;

	std::map<std::string, std::vector<std::pair<std::string, struct stat>>>::iterator it;
	for (it = filemap_.begin(); it != filemap_.end(); ++it) {
		std::string filepath{it->first};
		std::vector<std::pair<std::string, struct stat>> servers{it->second};

		if (servers.size() == 1) {
			filenames.push_back(filepath);
		} else {
			for (size_t i = 0; i < servers.size(); i++) {
				filenames.push_back(filepath + BEGIN_BRACE + servers[i].first + END_BRACE);
			}
		}
	}

	std::map<std::string, struct stat>::iterator dir_itr;
	for (dir_itr = directorymap_.begin(); dir_itr != directorymap_.end(); ++dir_itr) {
		filenames.push_back(dir_itr->first);
	}

	return filenames;
}

std::vector<std::pair<std::string, struct stat>> Disambiguater::disambiguatedFilepathsWithStats()
{
	std::vector<std::pair<std::string, struct stat>> filenames;

	std::map<std::string, std::vector<std::pair<std::string, struct stat>>>::iterator it;
	for (it = filemap_.begin(); it != filemap_.end(); ++it) {
		std::string filepath{it->first};
		std::vector<std::pair<std::string, struct stat>> servers{it->second};

		if (servers.size() == 1) {
			filenames.push_back(std::pair<std::string, struct stat>{filepath, servers[0].second});
		} else {
			for (size_t i = 0; i < servers.size(); i++) {
				std::string file = filepath + BEGIN_BRACE + servers[i].first + END_BRACE;
				struct stat stbuf = servers[i].second;
				filenames.push_back(std::pair<std::string, struct stat>{file, stbuf});
			}
		}
	}

	std::map<std::string, struct stat>::iterator dir_itr;
	for (dir_itr = directorymap_.begin(); dir_itr != directorymap_.end(); ++dir_itr) {
		filenames.push_back(*dir_itr);
	}

	return filenames;
}

void Disambiguater::clearPaths()
{
	filemap_.clear();
}

std::pair<std::string, std::string> Disambiguater::ambiguate(std::string path)
{
	size_t begin_position = path.find(BEGIN_BRACE);
	size_t end_position = path.find(END_BRACE);

	std::string server, filepath;

	if (begin_position == std::string::npos || end_position == std::string::npos) {
		server = "";
		filepath = path;
	} else {
		//Retrieve the server name
		size_t serverLoc = begin_position + BEGIN_BRACE.size();
		server = path.substr(serverLoc, end_position - serverLoc);

		//Retrieve the path
		filepath = path.substr(0, begin_position);
		filepath += path.substr(end_position + END_BRACE.size());
	}

	return std::pair<std::string, std::string>{server, filepath};
}

