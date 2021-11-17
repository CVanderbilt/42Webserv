#pragma once

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>

typedef enum http_parse_err
{
	ERR_HTTP_FATAL = -100,
	ERR_HTTP_EXTRA,
	ERR_HTTP_HEAD,
	ERR_HTTP_BODY
}	t_http_parse_err;

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
//		int			_content_size;

		void parse_method(void);
		void parse_head(void);
		void parse_body(void);
		void parse_body_multiform(void);

	public:
		size_t								max_size;
		parsing_status 						status;
		std::string							method;
		std::string							uri;
		std::string							protocol;
		std::map<std::string, std::string>	head;
		std::string							body;
		size_t								content_length;
		std::vector<Mult_Form_Data>			mult_form_data;
		Http_req(void);

		parsing_status parse_chunk(std::string chunk);
		static std::string status_to_str(parsing_status st);
};

std::ostream&   operator<<(std::ostream& os, const Http_req& obj);