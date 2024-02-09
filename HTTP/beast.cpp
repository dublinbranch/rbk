//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: Advanced server
//
//------------------------------------------------------------------------------

#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>

//in suse 15.5 version of gcc13.2.1 there is a false error inside so we suppress the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <boost/beast/http.hpp>
#pragma GCC diagnostic pop


#include <QCommandLineParser>
#include <QDebug>
#include <fmt/color.h>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/httpexception.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/filesystem/folder.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/rand/randutil.h"
#include "rbk/thread/threadstatush.h"

#include "PMFCGI.h"
#include "beast.h"
#include "router.h"

extern ThreadStatus                       threadStatus;
extern thread_local ThreadStatus::Status* localThreadStatus;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http  = beast::http;          // from <boost/beast/http.hpp>
namespace net   = boost::asio;          // from <boost/asio.hpp>
using tcp       = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using namespace std;

void init();

static const std::vector<std::string> errorPrefix{
    "SRLY Bad",
    "Inappropriate",
    "Interesting",
    "Unxepectedly Bad",
    "Predicatable",
    "Boring"
    "OMG!",
    "Another",
    "Again an",
    "Is time for another"
    "Look mom an"};

string_view randomErrorPrefix() {
	uint s = static_cast<uint>(errorPrefix.size());
	auto r = rand(0, s - 1);
	return errorPrefix[r];
}
string randomError() {
	static const std::string s = " Internal Server Error!";
	std::string              e(randomErrorPrefix());
	e.append(s);
	return e;
}

http::response<http::string_body> quickResponse(string&& msg, http::status status = http::status::internal_server_error, string mime = "text/html") {
	http::response<http::string_body> res;
	res.body() = msg;
	res.content_length(res.body().size());
	res.result(status);
	res.set(http::field::content_type, mime);
	return res;
}

void send(beast::tcp_stream& stream, const http::response<http::string_body>& msg) {
	beast::error_code ec;
	http::write(
	    stream,
	    std::move(msg),
	    ec);
}

void sendResponseToClient(beast::tcp_stream& stream, Payload& payload) {
	if (payload.alreadySent) {
		return;
	}

	payload.alreadySent = true;
	auto size           = payload.html.size();

	http::response<http::string_body> res;
	res.body() = std::move(payload.html);
	res.content_length(size);
	res.result(static_cast<http::status>(payload.statusCode));
	res.set(http::field::content_type, payload.mime);

	for (auto& [key, value] : payload.headers) {
		res.insert(key, value);
	}

	//No idea if is ok to have keep alive for an internal thing ? Benefit will be negligible
	res.keep_alive(false);

	//equivalente to fastcgi_close
	send(stream, res);
	registerFlushTime();
}

template <class Body, class Allocator>
void handle_request(
    beast::tcp_stream&                                   stream,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    const BeastConf*                                     conf) {

	localThreadStatus->state = ThreadState::Immediate;
	requestBeging();

	Payload payload;
	Router  router;
	PMFCGI  status;

	try {
		try { //Yes exception can throw exceptions!

			status.remoteIp = stream.socket().remote_endpoint().address().to_string();
			status.path     = req.target();
			status.url      = Url(status.path);
			// if (!isValidUTF8(status.path)) {
			// 	throw ExceptionV2(QSL("Invalid utf8 in the PATH %1").arg(base64this(status.path)));
			// }

			status.body = req.body();

			for (auto& h : req.base()) {
				status.headers.add(h.name_string(), h.value());
			}

			if (auto v = status.headers.get("remote_addr"); v) {
				status.remoteIp = v.val->toStdString();
			}

			if (conf->prePhase1) {
				conf->prePhase1(status, payload);
			}
			if (conf->loginManager) {
				if (!conf->loginManager(status, payload)) {
					sendResponseToClient(stream, payload);
					return;
				}
			}
			/*
			 * phase 1
			 * execute immediate
			 */
			router.immediate(status, conf, payload);

			/*
			 * phase 2
			 * send response to client
			 */
			sendResponseToClient(stream, payload);

			/*
			 * phase 3
			 * execute deferred
			 * slow stuff, logging
			 */
			localThreadStatus->state = ThreadState::Deferred;
			router.deferred();

		} catch (const exception& e) {
			payload.mime       = "text/html";
			payload.statusCode = 500;

			string msg = status.serializeMsg(e.what());

			string file;
			if (auto e2 = dynamic_cast<const ExceptionV2*>(&e); e2) {
				if (auto HE = dynamic_cast<const HttpException*>(&e); HE) {
					if (!HE->httpErrMsg.empty()) {
						payload.html = HE->httpErrMsg;
					}

					payload.statusCode = HE->statusCode;
				}
				if (!e2->skipPrint) {
					fmt::print("\n------\n{}", msg);
				}
				file = conf->logFolder + "/" + e2->getLogFile();
			} else {
				payload.html = randomError();
				file         = conf->logFolder + "/stdException.log";
				fmt::print("\n------\n{}", msg);
			}

			fileAppendContents("\n------\n " + msg, file);

		} catch (...) {
			auto msg = status.serializeMsg("unkown exception");
			fmt::print("\n------\n{}", msg);
			fileAppendContents("\n------\n " + msg, conf->logFolder + "/unkException.log");
		}

		//in case the exception happened before the socket close during the immediate stage
		sendResponseToClient(stream, payload);
	}

	//also the serialize is error prone in case of badly messed request
	catch (const exception& e) {
		auto msg = status.serializeMsg(e.what(), true);
		fileAppendContents("\n------\n " + msg, conf->logFolder + "/stdException.log");
	} catch (...) {
		auto msg = status.serializeMsg("unkown exception", true);
		fileAppendContents("\n------\n " + msg, conf->logFolder + "/unkException.log");
	}

	return;
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) {
	std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class http_session : public std::enable_shared_from_this<http_session> {
	//We change compared to the reference implementation the http_pipelining support
	//for our use case IS PROBABLY NEVER USED

	beast::tcp_stream  stream_;
	beast::flat_buffer buffer_;
	const BeastConf*   conf = nullptr;

	// The parser is stored in an optional container so we can
	// construct it from scratch it at the beginning of each new message.
	boost::optional<http::request_parser<http::string_body>> parser_;

      public:
	// Take ownership of the socket
	http_session(tcp::socket&& socket, const BeastConf* _conf)
	    : stream_(std::move(socket)), conf(_conf) {
	}

	// Start the session
	void
	run() {
		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(
		    stream_.get_executor(),
		    beast::bind_front_handler(
		        &http_session::do_read,
		        this->shared_from_this()));
	}

      private:
	void
	do_read() {
		// Construct a new parser for each message
		parser_.emplace();

		// Apply a reasonable limit to the allowed size
		// of the body in bytes to prevent abuse.
		parser_->body_limit(20000);
		parser_->header_limit(20000);

		// Set the timeout.
		stream_.expires_after(std::chrono::seconds(30));

		// Read a request using the parser-oriented interface
		http::async_read(
		    stream_,
		    buffer_,
		    *parser_,
		    beast::bind_front_handler(
		        &http_session::on_read,
		        shared_from_this()));
	}

	void
	on_read(beast::error_code ec, std::size_t bytes_transferred) {

		try {
			//Entry point
			boost::ignore_unused(bytes_transferred);

			// This means they closed the connection
			if (ec == http::error::end_of_stream) {
				return do_close();
			}

			if (ec) {
				return fail(ec, "read");
			}

			handle_request(stream_, parser_->release(), conf);
		} catch (...) {
			send(stream_, quickResponse(randomError()));
		}

		requestEnd();
	}

	void
	on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if (ec) {
			return fail(ec, "write");
		}

		if (close) {
			// This means we should close the connection, usually because
			// the response indicated the "Connection: close" semantic.
			return do_close();
		}
	}

	void
	do_close() {
		// Send a TCP shutdown
		beast::error_code ec;
		stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

		// At this point the connection is closed gracefully
	}
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
	net::io_context& ioc_;
	tcp::acceptor    acceptor_;
	const BeastConf* conf;

      public:
	listener(
	    net::io_context& ioc,
	    tcp::endpoint    endpoint,
	    const BeastConf* conf_)
	    : ioc_(ioc), acceptor_(net::make_strand(ioc)), conf(conf_) {
		beast::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if (ec) {
			fail(ec, "open");
			return;
		}

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec) {
			fail(ec, "set_option");
			return;
		}

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if (ec) {
			fmt::print(stderr, fmt::emphasis::bold | fg(fmt::color::red),
			           "Can not bind to {}:{} \n", endpoint.address().to_string(), endpoint.port());
			exit(2);
		}

		// Start listening for connections
		acceptor_.listen(net::socket_base::max_listen_connections, ec);
		if (ec) {
			fail(ec, "listen");
			return;
		}
	}

	// Start accepting incoming connections
	void
	run() {
		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(
		    acceptor_.get_executor(),
		    beast::bind_front_handler(
		        &listener::do_accept,
		        this->shared_from_this()));
	}

      private:
	void
	do_accept() {
		// The new connection gets its own strand
		acceptor_.async_accept(

		    net::make_strand(ioc_),
		    beast::bind_front_handler(
		        &listener::on_accept,
		        shared_from_this()));
	}

	void
	on_accept(beast::error_code ec, tcp::socket socket) {
		if (ec) {
			fail(ec, "accept");
		} else {
			// Create the http session and run it
			std::make_shared<http_session>(
			    std::move(socket),
			    conf)
			    ->run();
		}

		// Accept another connection
		do_accept();
	}
};

void Beast::listen() {
	okToRun();
	// The io_context is required for all I/O
	IOC = new net::io_context{conf.worker};

	// Create and launch a listening port
	listener_p = std::make_shared<listener>(
	    *IOC,
	    tcp::endpoint{net::ip::make_address(conf.address), conf.port},
	    &conf);

	listener_p->run();

	// Capture SIGINT to perform a clean shutdown
	//(if not already captured by other, which is quite rare so not under config)
	auto signals2block = new net::signal_set(*IOC, SIGINT, SIGTERM);

	signals2block->async_wait(
	    [&](beast::error_code const&, int) {
		    // Stop the `io_context`. This will cause `run()`
		    // to return immediately, eventually destroying the
		    // `io_context` and all of the sockets in it.
		    fmt::print("Stopping\n");
		    IOC->stop();
		    //remove the handler ?, else the next ctrl c will not terminate the program ?
		    signals2block->remove(SIGINT);
		    exit(0);
	    });

	signals2block_p = signals2block;
	fmt::print("Ready listening on http://{}:{}\n", conf.address, conf.port);

	// Run the I/O service on the requested number of threads
	for (auto i = conf.worker; i > 0; --i) {
		auto status = threadStatus.newStatus();

		auto& t = threads.emplace_back(new std::thread(
		    [status, this] {
			    //I have no idea how to get linux TID (thread id) from the posix one -.- so I have to resort to this
			    status->tid       = gettid();
			    localThreadStatus = status.get();
			    //and than launch to io handler
			    IOC->run();
		    }));

		status->state = ThreadState::Idle;
		status->info  = "just created";

		threadStatus.pool.insert({t->get_id(), status});
	}
	threadStatus.free = threadStatus.pool.size();

	// Block until all the threads exit
	for (auto& t : threads) {
		t->join();
	}
}

void Beast::listen(const BeastConf& conf_) {
	this->conf = conf_;
	listen();
}

#include <fmt/color.h>

void Beast::okToRun() const {
	if (conf.logFolder.empty()) {
		string str = "missing config file to run the Beast HTTP server, set one" + stacker();
		fmt::print(stderr, fg(fmt::color::red), str);
		exit(2);
	}
	RBK::mkdir(conf.logFolder);
}
