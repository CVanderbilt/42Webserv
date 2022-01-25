#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>


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
		std::string _aux_buff;
		int			_mfd_size;
		size_t		_max_size;
		std::string	_method;

		bool	parse_uri(std::string& line, int eol);
		void	parse_key_value_pair(std::string& line);
		void	parse_loop(void);
		void	parse_method(void);
		void	parse_head(void);
		void	parse_body(void);
		void	parse_body_multiform(void);

	public:
		parsing_status 						status;
		std::string							uri;
		std::string							file_uri;
		std::string							query_string;
		std::string							protocol;
		std::map<std::string, std::string>	head;
		std::string							body;
		size_t								content_length;
		std::vector<Mult_Form_Data>			mult_form_data;
		Http_req(size_t max_size_body);
		Http_req(Http_req const &copy);

		parsing_status parse_chunk(char* chunk, size_t bytes);
		static std::string status_to_str(parsing_status st);
		void		initialize(size_t max_size_body);
		size_t		getMaxSize();
		void		setMaxSize(size_t value);
		std::string	getMethod();
};

std::ostream&   operator<<(std::ostream& os, const Http_req& obj);