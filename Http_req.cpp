#include "Http_req.hpp"
#include <iostream>
#include <unistd.h>
#include <limits>

Http_req::Http_req(size_t max_size_body)
{
	initialize(max_size_body);
}

Http_req::Http_req(Http_req const &copy):
	_max_size(copy._max_size),
	_method(copy._method),
	_uri(copy._uri),
	_pars_stat(copy._pars_stat),
	_file_uri(copy._file_uri),
	_query_string(copy._query_string),
	_protocol(copy._protocol),
	_head(copy._head),
	_body(copy._body),
	_content_length(copy._content_length),
	_mult_form_data(copy._mult_form_data)
{}

void Http_req::initialize(size_t max_size_body)
{
	_mfd_size = 0;
	_max_size = max_size_body;
	_pars_stat = Http_req::PARSE_INIT;
	_file_uri = "";

	this->_head.clear();
	this->_body.clear();
}

std::string Http_req::status_to_str(parsing_status st)
{
	if (st == Http_req::PARSE_END)
		return ("parse ended");
	if (st == Http_req::PARSE_ERROR)
		return ("parse ended with errors");
	if (st == Http_req::PARSE_HEAD)
		return ("parsing headers");
	return ("parsing _body");
}

void Http_req::parse_body(void)
{
	size_t body_len = _body.length();
	if (body_len == _content_length) 								//(1)_body length correct -> end
			_pars_stat = PARSE_END;
	else if (body_len > _content_length) 							//(2)_body length greater than expected -> trim -> end
	{
		std::string aux_str = _body.substr(_content_length, _aux_buff.npos);
		aux_str.append(_aux_buff, 0, _aux_buff.length());
		_aux_buff = aux_str;
		_body = _body.substr(0, _content_length);
	}
	else if (_aux_buff.length() + body_len >= _content_length) 		//(3)_body length + buff greater or equal
	{																//  than expected add until _body length correct
		_body.append(_aux_buff, 0, _content_length - body_len);
		_aux_buff = _aux_buff.substr(_content_length - body_len, _aux_buff.npos);
	}
	else															//(4)_body + buff < expected -> simple addition
	{
		_body.append(_aux_buff, 0, _aux_buff.length());
		_aux_buff.clear();
		return ;
	}
	_pars_stat = PARSE_END;
}

void Http_req::parse_body_multiform(void)
{
	std::stringstream	ss;
	std::string			line;
	size_t 				pos_1, pos_2;
	bool				in_body = false;

	ss << _body;
	while (std::getline(ss, line))
	{
		if (line.compare("--" + _head["boundary"] + "\r") == 0)
		{
			Mult_Form_Data new_mfd;
			_mult_form_data.push_back(new_mfd);
			_mfd_size++;
			in_body = false;
		}
		if (!in_body)
		{
			line = toLowerString(line);
			if ((pos_1 = line.find(':')) != line.npos)
			{
				if (line.compare(0, pos_1, "content-disposition") == 0)
				{
					pos_2 = line.find(';');
					_mult_form_data[_mfd_size - 1].content_disposition = line.substr(pos_1 + 1, pos_2 - pos_1 - 1);
					if((pos_1 = line.find('=')) != line.npos)
					{
						pos_2 = line.find(';', pos_1);
						if (pos_2 != line.npos)
							_mult_form_data[_mfd_size - 1].name = line.substr(pos_1 + 2, pos_2 - 1 - pos_1 - 2);
						else
							_mult_form_data[_mfd_size - 1].name = line.substr(pos_1 + 2, line.size() - pos_1 - 4);
					}
					if((pos_1 = line.find('=', pos_1 + 1)) != line.npos)
						_mult_form_data[_mfd_size - 1].filename = line.substr(pos_1 + 2, line.size() - pos_1 - 4);
				}
				else if (line.compare(0, pos_1, "content-type") == 0)
					_mult_form_data[_mfd_size - 1].content_type = line.substr(pos_1 + 1, line.npos - pos_1 - 1);
			}
			else if (line == "\r")
				in_body = true;
		}
		else
		{
			if (line.compare(0, _head["boundary"].size() + 4, "--" + _head["boundary"] + "--") == 0)
			{
				in_body = false;
				_pars_stat = PARSE_END;
				break;
			}
			_mult_form_data[_mfd_size - 1].body.append(line, 0, line.length()).append("\n", 0, 1);
		}
	}
	if (_pars_stat != PARSE_END)
		_pars_stat = PARSE_ERROR;
}

bool Http_req::parse_uri(std::string& line, int eol)
{
	size_t sep = line.find(" ");
	if (sep == _aux_buff.npos)
		return (false);
	_uri = line.substr(eol + 1, sep - eol - 1);
	size_t pos_slash = _uri.find_last_of('/');
	size_t pos_qm = _uri.find('?');
	if (pos_qm > pos_slash)
		_file_uri = _uri.substr(pos_slash + 1, pos_qm - pos_slash - 1);
	else if (pos_slash != _uri.npos)
		_file_uri = _uri.substr(pos_slash + 1, _uri.npos);
	if (pos_qm != _uri.npos)
		_query_string = _uri.substr(pos_qm + 1, _uri.npos - pos_qm - 1);
	_protocol = line.substr(sep + 1, line.npos);
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
	_pars_stat = PARSE_ERROR;
	if (eol == _aux_buff.npos || eol == 0)
		return ;
	_method = line.substr(0, eol);
	
	line[eol] = '.';
	if (!parse_uri(line, eol) || _protocol != "HTTP/1.1")
		return ;
	_pars_stat = PARSE_HEAD;
}

void Http_req::parse_key_value_pair(std::string& line)
{
	size_t eol = line.find(":");
	if (eol == line.npos)
	{
		_pars_stat = PARSE_ERROR;
		return ;
	}
	std::string key = toLowerString(line.substr(0, eol));
	while (isspace(line[eol + 1]))
		eol++;
	line = toLowerString(line.substr(eol + 1, line.npos));
	if (_head.count(key))
		_head[key] = line.empty() ? _head[key] : _head[key] + ", " + line;
	else
		_head[key] = line;
	if (key == "content-type"  && line.compare(0, 19, "multipart/form-data") == 0)
	{
		_head[key] = "multipart/form-data";
		eol = line.find("=");
		line = line.substr(eol + 1, line.npos);
		_head["boundary"] = line;
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
			_pars_stat = PARSE_BODY;
			_content_length = 0;
			if (_head.count("content-length"))
				_content_length = std::atol(_head["content-length"].c_str());
			else
				_pars_stat = PARSE_END;
			break ;
		}
		eol = _aux_buff.find("\r\n");
	}
}

void Http_req::parse_loop(void)
{
	while(_pars_stat != PARSE_ERROR && _pars_stat != PARSE_END)
	{
		switch (_pars_stat)
		{
			case PARSE_INIT:
				parse_method();
				if (_aux_buff.length() > 0 && _pars_stat == PARSE_HEAD)
					continue ;
				break ;
			case PARSE_HEAD:
				parse_head();
				if (_aux_buff.length() > 0 && _pars_stat == PARSE_BODY)
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
	if (_pars_stat == PARSE_ERROR || _pars_stat == PARSE_END)
		return _pars_stat;
	_aux_buff.append(chunk, bytes);

	parse_loop();
	if (_body.length() > _max_size)
	{
		_pars_stat = PARSE_ERROR;
		return (_pars_stat);
	}
	if (_pars_stat == PARSE_END && _body != "" && _head["content-type"] == "multipart/form-data")
	{
		_pars_stat = PARSE_BODY;
		parse_body_multiform();
	}
	return (_pars_stat);
}

size_t	Http_req::getMaxSize() const
{
	return (_max_size);
}

void	Http_req::setMaxSize(size_t value)
{
	_max_size = value;
}

std::string	Http_req::getMethod() const
{
	return (_method);
}

std::string	Http_req::getUri() const
{
	return (_uri);
}

std::string	Http_req::getFileUri() const
{
	return (_file_uri);
}

std::string	Http_req::getQueryString() const
{
	return (_query_string);
}

std::string	Http_req::getProtocol() const
{
	return (_protocol);
}

std::map<std::string, std::string>	Http_req::getHead() const
{
	return (_head);
}

std::string	Http_req::getBody() const
{
	return (_body);
}

std::vector<Mult_Form_Data> Http_req::getMultFormData() const
{
	return (_mult_form_data);
}
