
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
	//We don't need to check if the map has the path in it, since
	//the map will just create a new vector if it doesn't contain 
	//the path.
	for (size_t i = 0; i < map_[path].size(); i++) {
		if (map_[path][i] == server) {
			return -EEXIST;
		}
	}

	map_[path].push_back(server);
	return 0;
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

	for (std::map<std::string, std::vector<std::string>>::iterator it = map_.begin();
			it != map_.end(); ++it) {
		std::string filepath{it->first};
		std::vector<std::string> servers{it->second};

		if (servers.size() == 1) {
			filenames.push_back(filepath);
		} else {
			for (size_t i = 0; i < servers.size(); i++) {
				filenames.push_back(filepath + BEGIN_BRACE + servers[i] + END_BRACE);
			}
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

