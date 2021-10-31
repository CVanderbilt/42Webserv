#pragma once

#include <map>
#include <string>
#include <iostream>

typedef enum http_parse_err
{
	ERR_HTTP_FATAL = -100,
	ERR_HTTP_EXTRA,
	ERR_HTTP_HEAD,
	ERR_HTTP_BODY
}	t_http_parse_err;

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
		int			_content_size;
		parsing_status status;

		void parse_method(void);
		void parse_head(void);
		void parse_body(void);

	public:
		std::string							method;
		std::string							uri;
		std::string							protocol;
		std::map<std::string, std::string>	head;
		std::string							body;
		int									content_length;
		Http_req(void);

		parsing_status parse_chunk(std::string chunk);
		static std::string status_to_str(parsing_status st);
};

std::ostream&   operator<<(std::ostream& os, const Http_req& obj);