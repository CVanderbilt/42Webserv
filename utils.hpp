#ifndef UTILS_HPP
# define UTILS_HPP

# include <vector>
# include <string>
# include <sys/stat.h>
# include <fstream>
# include <sstream>

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep);
bool		isPort(std::string p);
int			FileExists(std::string file);
std::string ExtractFile(std::string file);

#endif