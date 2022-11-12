#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <map>
#include <string>
struct Payload {
	bool                               alreadySent = false;
	unsigned                           statusCode  = 200;
	std::string                        html;
	std::string                        mime = "text/html";
	std::map<std::string, std::string> headers;
	void                               setStandardHeaders(bool addCors = true);
};

#endif // PAYLOAD_H
