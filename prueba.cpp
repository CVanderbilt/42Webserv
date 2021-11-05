#include <vector>
#include <iostream>
#include <poll.h>

int main()
{
	std::vector<pollfd> my_vector;

	my_vector.reserve(2);

	my_vector[0].fd = 1;
	my_vector[0].events = POLLOUT;

	std::cout << "my_vector[0].fd = " << my_vector[0].fd << std::endl;

}
