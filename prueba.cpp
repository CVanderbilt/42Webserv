#include <iostream>
#include <fstream>
#include <string>

int main()
{
	std::ifstream	ifs;
	std::string 	str;
	std::string 	src;
	int i = 0;

	ifs.open("rest.http");
	while (getline(ifs, str))
	{
		std::cout << "str " << i << " = " << str << std::endl;
		i++;
		src += str + "\n";
	}	

	std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
	std::cout << "src = " << src << std::endl;
}