#include <iostream>
#include <fstream>

int main()
{
	std::ofstream ofs("texto.txt");

	ofs << "hola mundo" << std::endl;
	
}