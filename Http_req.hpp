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
			PARSE_ONGOING,		//returned when head was not parsed.
			PARSE_HEAD,			//returned when head completely parsed but parsing was not completed.
			PARSE_END			//returned when parsing ended, attempt to parse more lines after this received will do nothing
		}	parsing_status;

	private:
		std::string _aux_buff;
		int			_content_size;
		parsing_status status;

		int parse_chunk_body(void);

	public:
		std::string							method;
		std::map<std::string, std::string>	head;
		std::string							body;
		int									content_length;
		Http_req(void);

		parsing_status parse_chunk(std::string chunk);
		static std::string status_to_str(parsing_status st);
};

std::ostream&   operator<<(std::ostream& os, const Http_req& obj);