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