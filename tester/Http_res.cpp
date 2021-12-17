#include "Http_res.hpp"
#include <iostream>
#include <unistd.h>
#include <limits>

Http_res::Http_res(size_t max_size_body)
{
	initialize(max_size_body);
}

Http_res::Http_res(Http_res const &copy):
	max_size(copy.max_size),
	status(copy.status),
	protocol(copy.protocol),
	head(copy.head),
	body(copy.body),
	content_length(copy.content_length)
{}

void Http_res::initialize(size_t max_size_body)
{
	_mfd_size = 0;
	max_size = max_size_body;
	status = Http_res::PARSE_INIT;

	this->head.clear();
	this->body.clear();
}

void Http_res::parse_body(void)
{
	size_t body_len = body.length();

	if (body_len > max_size)
	{
		status = PARSE_ERROR;
		return ;
	}
	if (body_len == content_length) 								//(1)body length correct -> end
			status = PARSE_END;
	else if (body_len > content_length) 							//(2)body length greater than expected -> trim -> end
	{
		std::string aux_str = body.substr(content_length, _aux_buff.npos);
		aux_str.append(_aux_buff, 0, _aux_buff.length());
		_aux_buff = aux_str;
		body = body.substr(0, content_length);
	}
	else if (_aux_buff.length() + body_len >= content_length) 		//(3)body length + buff greater or equal
	{																//  than expected add until body length correct
		body.append(_aux_buff, 0, content_length - body_len);
		_aux_buff = _aux_buff.substr(content_length - body_len, _aux_buff.npos);
	}
	else															//(4)body + buff < expected -> simple addition
	{
		body.append(_aux_buff, 0, _aux_buff.length());
		_aux_buff.clear();
		return ;
	}
	status = PARSE_END;
}

void Http_res::parse_method(void)
{
	size_t eol = _aux_buff.find("\r\n");
	if (eol == _aux_buff.npos)
		return ;

	std::string line = _aux_buff.substr(0, eol);
	_aux_buff = _aux_buff.substr(eol + 2, _aux_buff.npos);
			
	eol = line.find(" ");
	protocol = line.substr(0, eol);
	status_line = line.substr(eol + 1, std::string::npos);
	
	status = PARSE_HEAD;
}

void Http_res::parse_key_value_pair(std::string& line)
{
	size_t eol = line.find(":");
	if (eol == line.npos)
	{
		status = PARSE_ERROR;
		return ;
	}
	std::string key = line.substr(0, eol);
	while (isspace(line[eol + 1]))
		eol++;
	line = line.substr(eol + 1, line.npos);
	if (head.count(key))
		head[key] = line.empty() ? head[key] : head[key] + ", " + line;
	else
		head[key] = line;
}

void Http_res::parse_head(void)
{
	size_t eol = _aux_buff.find("\r\n");
	while (eol != _aux_buff.npos)
	{
		std::string line = _aux_buff.substr(0, eol);
		_aux_buff = _aux_buff.substr(eol + 2, _aux_buff.npos);

		if (eol != 0)
			parse_key_value_pair(line);
		else
		{
			status = PARSE_BODY;
			content_length = 0;
			if (head.count("Content-Length"))
				content_length = std::atol(head["Content-Length"].c_str());
			else if (head.count("content-length"))
				content_length = std::atol(head["content-length"].c_str());
			else
			{
				std::cout << "set status to end" << std::endl;
				status = PARSE_END;
			}
		}
		eol = _aux_buff.find("\r\n");
	}
}

void Http_res::parse_loop(void)
{
	while(status != PARSE_ERROR && status != PARSE_END)
	{
		switch (status)
		{
			case PARSE_INIT:
				//std::cout << "m";
				parse_method();
				if (_aux_buff.length() > 0 && status == PARSE_HEAD)
					continue ;
				break ;
			case PARSE_HEAD:
				//std::cout << "h";
				parse_head();
				if (_aux_buff.length() > 0 && status == PARSE_BODY)
					continue ;
				break ;
			case PARSE_BODY:
				//std::cout << "b";
				parse_body();
				break ;
			default:
				break ;
		}
		break ;
	}
	//std::cout << std::endl;
}

Http_res::parsing_status Http_res::parse_chunk(char* chunk, size_t bytes)
{
	//std::cout << std::endl << "length antes: " << _aux_buff.length() << std::endl;
	if (status == PARSE_ERROR || status == PARSE_END)
		return status;
	_aux_buff.append(chunk, bytes);
	//std::cout << "ahora: " << _aux_buff.length() << std::endl;

	parse_loop();
	//std::cout << "despues del loop: " << _aux_buff.length() << std::endl << std::endl;
	return (status);
}
