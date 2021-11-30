#include "Http_req.hpp"
#include <iostream>
#include <unistd.h>
#include <limits>

/*
*	returns a std::string containing the next line.
*	empty lines are returned as empty strings.
*	when ended it will return null.
*/

Http_req::Http_req(size_t max_size_body)
{
	initialize(max_size_body);
}

void Http_req::initialize(size_t max_size_body)
{
	_mfd_size = 0;
	max_size = max_size_body;
	status = Http_req::PARSE_INIT;

	this->head.clear();
	this->body.clear();
}

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

	if (body_len > max_size)
	{
		status = PARSE_ERROR;
		return ;
	}
	if (body_len == content_length) 								//(1)body length correct -> end
			status = PARSE_END;
	else if (body_len > content_length) 							//(2)body length greater than expected -> trim -> end
	{
		std::cout << "body len > content_length" << std::endl;
		std::string aux_str = body.substr(content_length, _aux_buff.npos);
		aux_str.append(_aux_buff, 0, _aux_buff.length());
		_aux_buff = aux_str;
		//creo que esto estaba mal(lo que hay comentado debajo), los bytes sobrantes no se meterían al final del buffer si no al principio,
		//_aux_buff += body.substr(content_length, _aux_buff.npos);	//removed chars from body added to buffer
		body = body.substr(0, content_length);
	}
	else if (_aux_buff.length() + body_len >= content_length) 		//(3)body length + buff greater or equal
	{																//  than expected add until body length correct
		std::cout << "aux + bodylen >= content_length" << std::endl;
		body.append(_aux_buff, 0, content_length - body_len);
		//body += _aux_buff.substr(0, content_length - body_len);
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

void Http_req::parse_body_multiform(void)
{
	std::stringstream	ss;
	std::string			line;
	size_t 				pos_1, pos_2;
	bool				in_body = false;

	ss << body;
	while (std::getline(ss, line))
	{
		if (line.compare("--" + head["boundary"] + "\r") == 0)
		{
			Mult_Form_Data new_mfd;
			mult_form_data.push_back(new_mfd);
			_mfd_size++;
			in_body = false;
		}
		if (!in_body)
		{
			if ((pos_1 = line.find(':')) != line.npos)
			{
				if (line.compare(0, pos_1, "Content-Disposition") == 0)
				{
					pos_2 = line.find(';');
					mult_form_data[_mfd_size - 1].content_disposition = line.substr(pos_1 + 1, pos_2 - pos_1 - 1);
					if((pos_1 = line.find('=')) != line.npos)
					{
						pos_2 = line.find(';', pos_1);
						if (pos_2 != line.npos)
							mult_form_data[_mfd_size - 1].name = line.substr(pos_1 + 2, pos_2 - 1 - pos_1 - 2);
						else
							mult_form_data[_mfd_size - 1].name = line.substr(pos_1 + 2, line.size() - pos_1 - 4);
					}
					if((pos_1 = line.find('=', pos_1 + 1)) != line.npos)
						mult_form_data[_mfd_size - 1].filename = line.substr(pos_1 + 2, line.size() - pos_1 - 4);
				}
				else if (line.compare(0, pos_1, "Content-Type") == 0)
					mult_form_data[_mfd_size - 1].content_type = line.substr(pos_1 + 1, line.npos - pos_1 - 1);
			}
			else if (line == "\r")
				in_body = true;
		}
		else
		{
			if (line.compare(0, head["boundary"].size() + 4, "--" + head["boundary"] + "--") == 0)
			{
				in_body = false;
				status = PARSE_END;
				break;
			}
			mult_form_data[_mfd_size - 1].body.append(line, 0, line.length()).append("\n", 0, 1);
	//		mult_form_data[_mfd_size - 1].body += line + "\n";
		}
	}
	if (status != PARSE_END)
		status = PARSE_ERROR;
}

void Http_req::parse_method(void)
{
	size_t eol = _aux_buff.find("\r\n");
	if (eol == _aux_buff.npos)
		return ;
	std::string line = _aux_buff.substr(0, eol);			//first line from chundk
	_aux_buff = _aux_buff.substr(eol + 2, _aux_buff.npos);	//first line substracted from chunk
			
	eol = line.find(" ");
	status = PARSE_HEAD;
	if (eol == _aux_buff.npos)
	{
		status = PARSE_ERROR;
		return ;
	}
	method = line.substr(0, eol);
	
	line[eol] = '.';
	size_t sep = line.find(" ");
	if (sep == _aux_buff.npos)
	{
		status = PARSE_ERROR;
		return ;
	}
	uri = line.substr(eol + 1, sep - eol - 1);
	
	protocol = line.substr(sep + 1, line.npos);
	if (protocol != "HTTP/1.1")
		status = PARSE_ERROR;
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
	while (isspace(line[eol + 1]))
		eol++;
	line = line.substr(eol + 1, line.npos);	//value
	if (head.count(key))	//key already exists
		head[key] = line.empty() ? head[key] : head[key] + ", " + line;
	else
		head[key] = line;
	if ((key == "content-type" || key == "Content-Type") && line.compare(0, 18, "multipart/form-data"))
	{
		head[key] = "multipart/form-data";
		eol = line.find("=");
		line = line.substr(eol + 1, line.npos);
		head["boundary"] = line;
	}
}

Http_req::parsing_status Http_req::parse_chunk(char* chunk, size_t bytes)
{
	if (status == PARSE_ERROR || status == PARSE_END)
		return status;
	_aux_buff.append(chunk, bytes);

	while(status != PARSE_ERROR && status != PARSE_END)
	{
		switch (status)
		{
			case PARSE_INIT:
			std::cout << "method" << std::endl;
				parse_method();
				if (_aux_buff.length() > 0)
					continue ;
				break ;
			case PARSE_HEAD:
			std::cout << "head" << std::endl;
				parse_head();
				if (_aux_buff.length() > 0)
					continue ;
				break ;
			case PARSE_BODY:
			std::cout << "body" << std::endl;
				parse_body();
				break ;
			default:
				break ;
		}
		break ;
	}
	if (status == PARSE_END && body != "" && (head["Content-Type"] == "multipart/form-data" || head["content-type"] == "multipart/form-data"))
	{
		status = PARSE_BODY;
		parse_body_multiform();
		std::cout << "body size = " << body.length() << std::endl;
		//std::cout << "body mdf size = " << mult_form_data[0].body.length() << std::endl;
	}
	return (status);
}

std::ostream&   operator<<(std::ostream& os, const Http_req& obj)
{
	os << "Method: " << obj.method << ", uri: " << obj.uri << ", protocol: " << obj.protocol << std::endl;
	os << "Head:" << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = obj.head.begin();
		it != obj.head.end();
		it++)
		os << it->first << ":" << it->second << std::endl;
	os << "Body:" << std::endl;
	os << obj.body << std::endl;
/*	if (obj.head.find("content-type")->second == "multipart/form-data")
	{
		os << std::endl << "XXXXXXXXXXXXXXXXXXXXX     Multipart/form-data   XXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		os << "vector mfd size = " << obj.mult_form_data.size() << std::endl;
		for (size_t i = 0; i < obj.mult_form_data.size(); i++)
		{
			os << "content_disposition:" << obj.mult_form_data[i].content_disposition << std::endl;
			os << "content_type:" << obj.mult_form_data[i].content_type << std::endl;
			os << "name:" << obj.mult_form_data[i].name << std::endl;
			os << "filename:" << obj.mult_form_data[i].filename << std::endl;
			os << "body:" << obj.mult_form_data[i].body << std::endl << std::endl;
		}
	}
*/	return (os);
}
