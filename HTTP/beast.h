#ifndef BEAST_H
#define BEAST_H

#include "beastConfig.h"
#include <chrono>

struct Beast {
	void      listen();
	void      listen(const BeastConf& conf_);
	BeastConf conf;

      private:
	void okToRun() const;
};

/* Serves one accepted connection as a listener worker does. One thread must run the io_context of the socket.
 * timeout: limit for each read and each write of the connection.
 * Tests use it for a session on their own io_context, without listen() and its signal handling. */
void serveHttpConnection(rbk::Http::WorkerSocket&& socket, const BeastConf* conf,
                         std::chrono::milliseconds timeout = std::chrono::seconds(30));

#endif // BEAST_H
