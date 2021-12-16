// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
# include <vector>
# include <map>
# include <string>
# include <sys/stat.h>
# include <fstream>
# include <sstream>
# include <cstdlib>
#include <stdint.h>
#include "Http_res.hpp"

#define PORT 8080

int	sock;
struct sockaddr_in serv_addr;

std::string ExtractFile(std::string filename)
{
	
	std::ostringstream contents;
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

    if (!in.is_open() || !in.good())
		throw std::exception();
	contents << in.rdbuf();
	in.close();
	return contents.str();
}

Http_res *getResponse()
{
	char buff[1025];
	Http_res *ret = new Http_res(-1);

	while (1)
	{
		int rb = recv(sock, buff, 1024, 0);
		if (rb < 0)
		{
			//std::cout << "rb < 0" << std::endl;
			break ;
		}
		if (rb == 0)
		{
			//std::cout << "rb == 0" << std::endl;
			return (ret);
		}
		Http_res::parsing_status st = ret->parse_chunk(buff, rb);
		//if (ret->parse_chunk(buff, rb) == Http_res::PARSE_ERROR)
		if (st == Http_res::PARSE_END || st == Http_res::PARSE_ERROR)
			return (ret);
	}
	delete ret;
	return (NULL);
}

bool	assert_str(const std::string& a, const std::string& b)
{
	if (a == b)
		return (true);
	std::cout << ">" << a << "< != >" << b << "<" << std::endl;
	return (false);
}

bool check_instruction(Http_res *res, std::string instruction)
{
	size_t l = instruction.length();
	if (l < 2)
	{
		std::cerr << "Empty or too short instruction" << std::endl;
		return (false);
	}
	char c = instruction[0];
	std::string key;
	size_t pos;
	switch (c)
	{
	case 'S':
	case 's':
		return (assert_str(res->status_line, instruction.substr(1, std::string::npos)));
	case 'H':
	case 'h':
		pos = instruction.find(" ");
		key = instruction.substr(1, pos - 1);
		if (res->head.count(key))
		{
			if (pos != std::string::npos)
				return (assert_str(res->head[key], instruction.substr(pos + 1, std::string::npos)));
			return (true);
		}
		std::cerr << "missign key: >" << key << "<" << std::endl;
		break ;
	default:
		break;
	}
	return (false);
}

bool checks(Http_res *res, std::string& instructions)
{
	size_t pos;
	size_t last_pos = 0;
	bool ret = true;

	do
	{
		pos = instructions.find("\n");
		std::string single_inst = instructions.substr(last_pos, pos - last_pos);
		instructions[pos] = '.';
		last_pos = pos + 1;
		if (single_inst == "")
			continue ;
		//std::cout << "pos: " << pos << std::endl;
		//if (pos != std::string::npos)
		//	std::cout << "in pos: >" << instructions[pos] << "<" << std::endl; 
	//	std::cout << "(debug)" << "instruction: >" << single_inst << "<" << std::endl;
		if (!check_instruction(res, single_inst))
			ret = false;
	} while (pos != std::string::npos);
	return (ret);
}

void show_str(const std::string& s)
{
	std::cout << ">";
	for (int i = 0; i < s.length(); i++)
	{
		switch (s[i])
		{
		case '\n':
			std::cout << "\\n";
			break ;
		case '\r':
			std::cout << "\\r";
			break;
		default:
			std::cout << s[i];
		}
	}
	std::cout << "<";
}

void mod_msg(std::string& msg)
{
	size_t pos = msg.find("\n\n");
	std::string ret = "";

	for (int i = 0; i < msg.length(); i++)
	{
		if (i < pos && msg[i] == '\n')
			ret += '\r';
		if (i == pos && msg[i] == '\n')
		{
			ret += '\r';
			ret += msg[i++];
			ret += '\r';
		}
		ret += msg[i];
	}
	msg = ret;
	std::cout << "will send: ";
	show_str(ret);
	std::cout << std::endl;
}

void tests(std::string file_path)
{
	std::string file_string = ExtractFile(file_path);
	size_t pos = 0;
	size_t i = 0;

	std::cout << "========================================================================" << std::endl;
	while (file_string != "")
	{
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	{
    	    printf("\n Socket creation error \n");
        	return ;
    	}
   
    	serv_addr.sin_family = AF_INET;
    	serv_addr.sin_port = htons(PORT);
       
    // Convert IPv4 and IPv6 addresses from text to binary form
    	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    	{
    	    printf("\nInvalid address/ Address not supported \n");
    	    return ;
    	}
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    	{
    	    printf("\nConnection Failed \n");
    	    return ;
    	}
		pos = file_string.find("###");
		std::string msg = file_string.substr(0, pos);
		mod_msg(msg);
		if (pos != std::string::npos)
			file_string = file_string.substr(pos + 3, std::string::npos);
		else
			file_string = "";
		if (msg == "\n")
			continue ;
		size_t eol = msg.find('\n');
		std::cout << "test(" << i++ << "): ";
		show_str(msg.substr(0, msg.find('\n') + (eol + 1 < msg.length() ? 1 : 0)));
		std::cout << ' ';
		//std::cout << std::endl;
		send(sock, msg.c_str(), msg.length(), 0);
		Http_res *req = getResponse();

		if (req == NULL)
		{
			std::cerr << "memory error!!!" << std::endl;
			close (sock);
			return ;
		}
		if (file_string == "")
		{
			delete req;
			close(sock);
			break ;
		}
		pos = file_string.find("###");
		msg = file_string.substr(0, pos);

		if (pos != std::string::npos)
			file_string = file_string.substr(pos + 3, std::string::npos);
		else
			file_string = "";

		if (req->status != Http_res::PARSE_END)
			std::cerr << "Parse error" << std::endl;

		if (!checks(req, msg))
			std::cerr << "failed" << std::endl;
		else
			std::cerr << "succes" << std::endl;
		delete req;
		pos = file_string.find('\n');
		if (pos == msg.npos)
			break ;
		file_string = file_string.substr(pos + 1, file_string.npos);
		std::cout << "========================================================================" << std::endl;
		close(sock);
	}
	std::cout << "========================================================================" << std::endl;

}

int main(int argc, char const *argv[])
{
	if (argc < 2)
	{
		std::cerr << "missing file path" << std::endl;
		return (1);
	}
   
	try
	{
		/* code */
		tests(argv[1]);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
    //send(sock , hello , strlen(hello) , 0 );
    //printf("Hello message sent\n");
    //valread = read( sock , buffer, 1024);
    //printf("%s\n",buffer );

    return 0;
}