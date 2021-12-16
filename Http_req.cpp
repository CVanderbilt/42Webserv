#include "Http_req.hpp"
#include <iostream>
#include <unistd.h>
#include <limits>

Http_req::Http_req(size_t max_size_body)
{
	initialize(max_size_body);
}

Http_req::Http_req(Http_req const &copy):
	max_size(copy.max_size),
	status(copy.status),
	method(copy.method),
	uri(copy.uri),
	file_uri(copy.file_uri),
	query_string(copy.query_string),
	protocol(copy.protocol),
	head(copy.head),
	body(copy.body),
	content_length(copy.content_length),
	mult_form_data(copy.mult_form_data)
{}

void Http_req::initialize(size_t max_size_body)
{
	_mfd_size = 0;
	max_size = max_size_body;
	status = Http_req::PARSE_INIT;
	file_uri = "";

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
		std::string aux_str = body.substr(content_length, _aux_buff.npos);
		aux_str.append(_aux_buff, 0, _aux_buff.length());
		_aux_buff = aux_str;
		body = body.substr(0, content_length);
	}
	else if (_aux_buff.length() + body_len >= content_length) 		//(3)body length + buff greater or equal
	{																//  than expected add until body length correct
		std::cout << "aux + bodylen >= content_length" << std::endl;
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
		}
	}
	if (status != PARSE_END)
		status = PARSE_ERROR;
}

bool Http_req::parse_uri(std::string& line, int eol)
{
	size_t sep = line.find(" ");
	if (sep == _aux_buff.npos)
		return (false);
	uri = line.substr(eol + 1, sep - eol - 1);
	size_t pos_slash = uri.find_last_of('/');
	size_t pos_qm = uri.find('?');
	if (pos_qm > pos_slash)
		file_uri = uri.substr(pos_slash + 1, pos_qm - pos_slash - 1);
	else if (pos_slash != uri.npos)
		file_uri = uri.substr(pos_slash + 1, uri.npos);
	if (pos_qm != uri.npos)
		query_string = uri.substr(pos_qm + 1, uri.npos - pos_qm - 1);
	protocol = line.substr(sep + 1, line.npos);
	return (true);
}

void Http_req::parse_method(void)
{
	size_t eol = _aux_buff.find("\r\n");
	if (eol == _aux_buff.npos)
		return ;
	std::string line = _aux_buff.substr(0, eol);
	_aux_buff = _aux_buff.substr(eol + 2, _aux_buff.npos);
			
	eol = line.find(" ");
	status = PARSE_ERROR;
	if (eol == _aux_buff.npos || eol == 0)
		return ;
	method = line.substr(0, eol);
	
	line[eol] = '.';
	if (!parse_uri(line, eol) || protocol != "HTTP/1.1")
		return ;
	status = PARSE_HEAD;
}

void Http_req::parse_key_value_pair(std::string& line)
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
	if ((key == "content-type" || key == "Content-Type") && line.compare(0, 19, "multipart/form-data") == 0)
	{
		head[key] = "multipart/form-data";
		eol = line.find("=");
		line = line.substr(eol + 1, line.npos);
		head["boundary"] = line;
	}
}

void Http_req::parse_head(void)
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
				status = PARSE_END;
			break ;
		}
		eol = _aux_buff.find("\r\n");
	}
}

void Http_req::parse_loop(void)
{
	while(status != PARSE_ERROR && status != PARSE_END)
	{
		switch (status)
		{
			case PARSE_INIT:
				parse_method();
				if (_aux_buff.length() > 0 && status == PARSE_HEAD)
					continue ;
				break ;
			case PARSE_HEAD:
				parse_head();
				if (_aux_buff.length() > 0 && status == PARSE_BODY)
					continue ;
				break ;
			case PARSE_BODY:
				parse_body();
				break ;
			default:
				break ;
		}
		break ;
	}
}

Http_req::parsing_status Http_req::parse_chunk(char* chunk, size_t bytes)
{
	if (status == PARSE_ERROR || status == PARSE_END)
		return status;
	_aux_buff.append(chunk, bytes);

	parse_loop();	
	if (status == PARSE_END && body != "" &&
		(head["Content-Type"] == "multipart/form-data" ||
		head["content-type"] == "multipart/form-data"))
	{
		status = PARSE_BODY;
		parse_body_multiform();
	}
	return (status);
}
