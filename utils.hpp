#ifndef UTILS_HPP
# define UTILS_HPP

# include <vector>
# include <string>

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep);
bool isPort(std::string p);

#endif