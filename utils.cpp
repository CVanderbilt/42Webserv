#include "utils.hpp"

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep)
{
	std::vector<std::string> ret;
	size_t start = 0;
	size_t sep_size = sep.size();

	while (1)
	{
		size_t pos = str.find(sep);
		
		ret.push_back(str.substr(0, pos));
		if (pos == str.npos)
			break ;
		str.erase(0, pos + 1);
	}
	return (ret);
}

bool isPort(std::string p)
{
	for (int i = 0; i < p.size(); i++)
		if (!std::isdigit(p[i]))
			return (false);
	int n = std::stoi(p);
	if (n < 0 || n > 65535)
		return (false);
	return (true);
}

int FileExists(std::string file)
{
	struct stat st;

	return (stat(file.c_str(), &st));
}

std::string ExtractFile(std::string file)
{
	std::string			buf;
	std::ifstream		ifs(file);
	std::stringstream	stream;

	if (ifs.is_open())
	{
		while (std::getline(ifs, buf))
			stream << buf << "\n";
		ifs.close();
	}
	return (stream.str());
}