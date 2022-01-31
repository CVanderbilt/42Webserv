#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include "utils.hpp"

struct Mult_Form_Data
{
	std::string		content_disposition;
	std::string		content_type;
	std::string		name;
	std::string		filename;
	std::string		body;
};

class Http_req
{
	public:
	typedef enum e_parsing_status
		{
			PARSE_ERROR,		//error
			PARSE_INIT,			//initial status, the first thing to parse is the method
			PARSE_HEAD,			//parsing header
			PARSE_BODY,			//parsing body
			PARSE_END			//returned when parsing ended, attempt to parse more lines after this received will do nothing
		}	parsing_status;

	private:
		std::string 						_aux_buff;
		int									_mfd_size;
		size_t								_max_size;
		std::string							_method;
		std::string							_uri;
		parsing_status 						_pars_stat;
		std::string							_file_uri;
		std::string							_query_string;
		std::string							_protocol;
		std::map<std::string, std::string>	_head;
		std::string							_body;
		size_t								_content_length;
		std::vector<Mult_Form_Data>			_mult_form_data;

		bool	parse_uri(std::string& line, int eol);
		void	parse_key_value_pair(std::string& line);
		void	parse_loop(void);
		void	parse_method(void);
		void	parse_head(void);
		void	parse_body(void);
		void	parse_body_multiform(void);
		static std::string status_to_str(parsing_status st);

	public:
		Http_req(size_t max_size_body);
		Http_req(Http_req const &copy);

		parsing_status	parse_chunk(char* chunk, size_t bytes);
		void			initialize(size_t max_size_body);
		size_t			getMaxSize() const;
		void			setMaxSize(size_t value);
		std::string		getMethod() const;
		std::string		getUri() const;
		std::string		getFileUri() const;
		std::string		getQueryString() const;
		std::string		getProtocol() const;
		std::string		getBody() const;
		std::map<std::string, std::string>	getHead() const;
		std::vector<Mult_Form_Data> getMultFormData() const;
};
