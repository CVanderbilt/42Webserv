#include <iostream>
#include <string>

int main()
{
	std::string str = "hola mundo";
	
	std::cout << "str.find(""a"") = " << str.find("a") << std::endl;
	std::cout << "str.find(""a "") = " << str.find("a ") << std::endl;
	std::cout << "str.substr(3) = " << str.substr(3) << std::endl;
}