
#ifndef DISAMBIGUATER_HPP_INC
#define DISAMBIGUATER_HPP_INC

#include <map>
#include <string>
#include <vector>
#include "constants.hpp"

class Disambiguater {
public:
	Disambiguater();

	//Returns EEXIST if the path/server combo is already in the map.
	agerr_t addFilepath(std::string path, std::string server);

	agerr_t addFilepaths(std::vector<std::string> paths, std::string server);

	//Returns a vector of disambiguated filepaths
	std::vector<std::string> disambiguatedFilepaths();

	//Clears all file/server combos from the map.
	void clearPaths();

	//Ambiguates a file by returning the {server, filepath} pair
	static std::pair<std::string, std::string> ambiguate(std::string path);
	
private:

	std::map<std::string, std::vector<std::string>> map_;

	static const std::string BEGIN_BRACE;
	static const std::string END_BRACE;
};

#endif
