
#include "disambiguater.hpp"
#include <errno.h>

const std::string Disambiguater::BEGIN_BRACE = "{:";
const std::string Disambiguater::END_BRACE = ":}";

Disambiguater::Disambiguater()
	:map_{}
{
	//Nothing to do here...
}

agerr_t Disambiguater::addFilepath(std::string path, std::string server)
{
	std::vector<std::string> servers;
	
	//Check if the map contains the path.
	std::map<std::string, std::vector<std::string>>::iterator it = map_.find(path);
	if (it == map_.end()) {
		map_.insert(std::pair<std::string, std::vector<std::string>>{path, servers});
	}
	else {
		servers = map_[path];
		for (size_t i = 0; i < servers.size(); i++) {
			if (servers[i] == server) {
				return -EEXIST;
			}
		}
	}

	servers.push_back(server);
	return 0;
}

std::vector<std::string> Disambiguater::disambiguatedFilepaths()
{
	std::vector<std::string> filenames;

	for (std::map<std::string, std::vector<std::string>>::iterator it = map_.begin();
			it != map_.end(); ++it) {
		std::string filepath{it->first};
		std::vector<std::string> servers{it->second};
		for (size_t i = 0; i < servers.size(); i++) {
			filenames.push_back(filepath + BEGIN_BRACE + servers[i] + END_BRACE);
		}
	}

	return filenames;
}

void Disambiguater::clearPaths()
{
	map_.clear();
}

std::pair<std::string, std::string> Disambiguater::ambiguate(std::string path)
{
	size_t begin_position = path.rfind(BEGIN_BRACE);
	size_t end_position = path.rfind(END_BRACE);

	if (begin_position == std::string::npos || end_position == std::string::npos) {
		return std::pair<std::string, std::string>{path,""};
	}

	std::string filepath = path.substr(0, begin_position);
	size_t serverLoc = begin_position + BEGIN_BRACE.size();
	std::string server = path.substr(serverLoc, end_position - serverLoc);

	return std::pair<std::string, std::string>{filepath, server};
}

