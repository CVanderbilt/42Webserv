#include "Http_req.hpp"
#include <iostream>
#include <unistd.h>

/*
*	returns a std::string containing the next line.
*	empty lines are returned as empty strings.
*	when ended it will return null.
*/

Http_req::Http_req(void): status(Http_req::PARSE_INIT) {}

std::string Http_req::status_to_str(parsing_status st)
{
	if (st == Http_req::PARSE_END)
		return ("parse ended");
	if (st == Http_req::PARSE_ERROR)
		return ("parse ended with errors");
	if (st == Http_req::PARSE_HEAD)
		return ("parsing headers");
	return ("parsing body");
}

/*
*	This function add the content of _aux_buff into body up to
*	content_length bytes, when end 1 is returned, otherwise
*	0 is returned.
*/
void Http_req::parse_body(void)
{
	size_t body_len = body.length();

	if (body_len == content_length) 							//(1)body length correct -> end
		status = PARSE_END;
	else if (body_len > content_length) 						//(2)body length greater than expected -> trim -> end
		body = body.substr(0, content_length);
	else if (_aux_buff.length() + body_len >= content_length) 	//(3)body length + buff greater or equal
		body += _aux_buff.substr(0, content_length - body_len);	//  than expected add until body length correct
	else														//(4)body + buff < expected -> simple addition
	{
		body += _aux_buff;
		_aux_buff.clear();
		return ;
	}
	status = PARSE_END;
}

void Http_req::parse_method(void)
{
	size_t eol = _aux_buff.find("\r\n");
	if (eol == _aux_buff.npos)
		return ;
	std::string line = _aux_buff.substr(0, eol);			//first line from chundk
	_aux_buff = _aux_buff.substr(eol + 2, _aux_buff.npos);	//first line substracted from chunk
			
	method = line;
	status = PARSE_HEAD;
}

void Http_req::parse_head(void)
{
	size_t eol = _aux_buff.find("\r\n");
	if (eol == _aux_buff.npos)
		return ;
	std::string line = _aux_buff.substr(0, eol);			//first line from chundk
	_aux_buff = _aux_buff.substr(eol + 2, _aux_buff.npos);	//first line substracted from chunk

	if (eol == 0) //empty line -> head completed
	{
		status = PARSE_BODY;
		content_length = 0;
		if (head.count("Content-Length"))
			content_length = std::atol(head["Content-Length"].c_str());
		else if (head.count("content-length"))
			content_length = std::atol(head["content-length"].c_str());
		else
			status = PARSE_END;
		return ;
	}
	/*
	*	Processing key:value header pair
	*/
	eol = line.find(":");
	if (eol == line.npos)	//key:value pair not correctly formatted -> error
	{
		status = PARSE_ERROR;
		return ;
	}
	std::string key = line.substr(0, eol);	//key
	line = line.substr(eol + 1, line.npos);	//value
	if (head.count(key))	//key already exists
		head[key] = line.empty() ? head[key] : head[key] + ", " + line;
	else
		head[key] = line;
}


Http_req::parsing_status Http_req::parse_chunk(std::string chunk)
{
	if (status == PARSE_ERROR || status == PARSE_END)
		return status;
	_aux_buff += chunk;

	while(status != PARSE_ERROR && status != PARSE_END)
	{
		switch (status)
		{
			case PARSE_INIT:
				parse_method();
				continue ;
			case PARSE_HEAD:
				parse_head();
				continue ;
			case PARSE_BODY:
				parse_body();
				continue ;
			default:
				break ;
		}
	}
	//getchar();
	if (status == PARSE_END || status == PARSE_ERROR)
		_aux_buff.clear();
	return (status);
}

std::ostream&   operator<<(std::ostream& os, const Http_req& obj)
{
	os << "Method: " << obj.method << std::endl;
	os << "Head:" << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = obj.head.begin();
		it != obj.head.end();
		it++)
		os << it->first << ":" << it->second << std::endl;
	os << "Body:" << std::endl;
	os << obj.body << std::endl;
	return (os);
}