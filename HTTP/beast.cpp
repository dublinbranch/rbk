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

#include <boost/algorithm/string.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <QCommandLineParser>
#include <QDebug>
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "fmt/color.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/httpexception.h"
#include "rbk/filesystem//filefunction.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/ClickHouseException.h"
#include "rbk/misc/b64.h"
#include "rbk/string/UTF8Util.h"
#include "rbk/thread/threadstatush.h"

#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/router.h"
#include "rbk/defines/stringDefine.h"

extern ThreadStatus                       threadStatus;
extern thread_local ThreadStatus::Status* localThreadStatus;

bool skipCatchPrint(const ExceptionV2& exception) {
	return 1;
};

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http  = beast::http;          // from <boost/beast/http.hpp>
namespace net   = boost::asio;          // from <boost/asio.hpp>
using tcp       = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using namespace std;

void init();

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <
    class Body, class Allocator,
    class Send>
void handle_request(
    PMFCGI&                                              status,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&&                                               send) {

	localThreadStatus->state = ThreadState::Immediate;
	/*
	 * phase 1
	 * execute immediate
	 */
	Payload payload;

	Router router;
	try {
		try {
			status.path = req.target();
			status.body = req.body();

			if (!isValidUTF8(status.path)) {
				throw ExceptionV2(QSL("Invalid utf8 in key %1").arg(base64this(status.path)));
			}

			auto& header = req.base();
			for (auto& h : header) {
				status.headers.add(h.name_string(), h.value());
			}
			status.extractCookies();

			payload = router.immediate(status);
		} catch (const HttpException& e) {
			payload.mime       = "text/html";
			payload.html       = status.serializeMsg(e.what());
			payload.statusCode = 400;
			if (!skipCatchPrint(e)) {
				fmt::print("\n------\n{}", payload.html);
			}
			//			fileAppendContents("\n------\n " + payload.html, QSL("%1/httpException.log").arg(conf().log.folder));
		} catch (const exception& e) {
			auto msg = status.serializeMsg(e.what());
			fmt::print("\n------\n{}", msg);

			auto exceptionP = dynamic_cast<const ClickHouseException*>(&e);
			auto logFile    = QSL("stdException.log");
			if (exceptionP) {
				logFile = QSL("clickhouse.log");
			}
			//			fileAppendContents("\n------\n " + msg, QSL("%1/%2")
			//			                                            .arg(conf().log.folder)
			//			                                            .arg(logFile));

			if (status.debug > 0) {
				// debug
				payload.html       = msg;
				payload.statusCode = 400;
			} else {
				// normal execution
				payload.html       = "Internal server error"s;
				payload.statusCode = 500;
			}
		} catch (...) {
			auto msg = status.serializeMsg("unkown exception");
			fmt::print("\n------\n{}", msg);
			//			fileAppendContents("\n------\n " + msg, QSL("%1/unkException.log").arg(conf().log.folder));

			if (status.debug > 0) {
				// debug
				payload.html       = msg;
				payload.statusCode = 400;
			} else {
				// normal execution
				payload.html       = "Internal server error"s;
				payload.statusCode = 500;
			}
		}
	} catch (const exception& e) {
		//		fileAppendContents("\n------\n " + QString(e.what()), QSL("%1/%2")
		//		                                                          .arg(conf().log.folder)
		//		                                                          .arg("stdException.log"));

		if (status.debug > 0) {
			// debug
			payload.html       = e.what();
			payload.statusCode = 400;
		} else {
			// normal execution
			payload.html       = "Internal server error, double exception"s;
			payload.statusCode = 500;
		}
	} catch (...) {
		payload.html       = "Internal server error, double exception"s;
		payload.statusCode = 500;
	}

	localThreadStatus->state  = ThreadState::Deferred;
	auto sendResponseToClient = [&]() {
		http::response<http::string_body> res;
		res.content_length(payload.html.size());
		res.body() = std::move(payload.html);
		res.result(static_cast<http::status>(payload.statusCode));
		res.set(http::field::content_type, payload.mime);

		for (auto& [key, value] : payload.headers) {
			res.set(key, value);
		}

		//No idea if is ok to have keep alive for an internal thing ? Benefit will be negligible
		res.keep_alive(false);

		//equivalente to fastcgi_close
		send(std::move(res));
		registerFlushTime();
	};

	/*
	 * phase 2
	 * send response to client
	 */
	if (!status.debug) {
		// normal execution
		// first we send the result to the client and then, we execute deferred
		sendResponseToClient();
	}

	/*
	 * phase 3
	 * execute deferred
	 */

	//slow stuff, logging ecc ecc
	try {
		router.deferred();
	} catch (const HttpException& e) {
		auto msg = status.serializeMsg(e.what());
		if (status.debug) {
			if (!skipCatchPrint(e)) {
				fmt::print("\n------\n{}", msg);
			}
		}

		//fileAppendContents("\n------\n " + msg, QSL("%1/httpException.log").arg(conf().log.folder));
	} catch (const exception& e) {
		std::string msg = status.serializeMsg(e.what());
		if (status.debug) {
			fmt::print("\n------\n{}", msg);
		}

		auto exceptionP = dynamic_cast<const ClickHouseException*>(&e);
		auto logFile    = QSL("stdException.log");
		if (exceptionP) {
			logFile = QSL("clickhouse.log");
		}

		//		fileAppendContents("\n------\n " + msg, QSL("%1/%2")
		//		                                            .arg(conf().log.folder)
		//		                                            .arg(logFile));
	} catch (...) {
		auto type = currentExceptionTypeName();
		auto err  = QSL("unkown exception, type is %1").arg(type);
		auto msg  = status.serializeMsg(err);

		if (status.debug) {
			payload.html += msg;
			payload.statusCode = 400;
			payload.mime       = "text/html";

			fmt::print("\n------\n{}", msg);
		}

		//	fileAppendContents("\n------\n " + msg, QSL("%1/unkException.log").arg(conf().log.folder));
	}

	// only if debug active we send response here
	if (status.debug) {
		sendResponseToClient();
	}
	//-----------------------------

	return;
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) {
	std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class http_session : public std::enable_shared_from_this<http_session> {
	// This queue is used for HTTP pipelining.
	class queue {
		enum {
			// Maximum number of responses we will queue
			limit = 8
		};

		// The type-erased, saved work item
		struct work {
			virtual ~work()           = default;
			virtual void operator()() = 0;
		};

		http_session&                      self_;
		std::vector<std::unique_ptr<work>> items_;

	      public:
		explicit queue(http_session& self)
		    : self_(self) {
			static_assert(limit > 0, "queue limit must be positive");
			items_.reserve(limit);
		}

		// Returns `true` if we have reached the queue limit
		bool
		is_full() const {
			return items_.size() >= limit;
		}

		// Called when a message finishes sending
		// Returns `true` if the caller should initiate a read
		bool
		on_write() {
			BOOST_ASSERT(!items_.empty());
			auto const was_full = is_full();
			items_.erase(items_.begin());
			if (!items_.empty())
				(*items_.front())();
			return was_full;
		}

		// Called by the HTTP handler to send a response.
		template <bool isRequest, class Body, class Fields>
		void
		operator()(http::message<isRequest, Body, Fields>&& msg) {
			// This holds a work item
			struct work_impl : work {
				http_session&                          self_;
				http::message<isRequest, Body, Fields> msg_;

				work_impl(
				    http_session&                            self,
				    http::message<isRequest, Body, Fields>&& msg)
				    : self_(self), msg_(std::move(msg)) {
				}

				void
				operator()() {
					http::async_write(
					    self_.stream_,
					    msg_,
					    beast::bind_front_handler(
					        &http_session::on_write,
					        self_.shared_from_this(),
					        msg_.need_eof()));
				}
			};

			// Allocate and store the work
			items_.push_back(
			    boost::make_unique<work_impl>(self_, std::move(msg)));

			// If there was no previous work, start this one
			if (items_.size() == 1)
				(*items_.front())();
		}
	};

	beast::tcp_stream                  stream_;
	beast::flat_buffer                 buffer_;
	std::shared_ptr<std::string const> doc_root_;
	queue                              queue_;

	// The parser is stored in an optional container so we can
	// construct it from scratch it at the beginning of each new message.
	boost::optional<http::request_parser<http::string_body>> parser_;

      public:
	// Take ownership of the socket
	http_session(
	    tcp::socket&& socket)
	    : stream_(std::move(socket)), queue_(*this) {
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

			PMFCGI status;
			status.remoteIp = stream_.socket().remote_endpoint().address().to_string();
			// Send the response
			requestBeging();
			handle_request(status, parser_->release(), queue_);
		} catch (...) {
			//TODO e qui ?
		}

		requestEnd();

		// If we aren't at the queue limit, try to pipeline another request
		if (!queue_.is_full())
			do_read();
	}

	void
	on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "write");

		if (close) {
			// This means we should close the connection, usually because
			// the response indicated the "Connection: close" semantic.
			return do_close();
		}

		// Inform the queue that a write completed
		if (queue_.on_write()) {
			// Read another request
			do_read();
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

      public:
	listener(
	    net::io_context& ioc,
	    tcp::endpoint    endpoint)
	    : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
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
			auto msg = fmt::format(fmt::emphasis::bold | fg(fmt::color::red),
			                       "Can not bind to {}:{} \n", endpoint.address().to_string(), endpoint.port());
			throw msg;
		}

		// Start listening for connections
		acceptor_.listen(
		    net::socket_base::max_listen_connections, ec);
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
			    std::move(socket))
			    ->run();
		}

		// Accept another connection
		do_accept();
	}
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
	QCoreApplication application(argc, argv);

	// commandline parser
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOption({{"M", "executionMode"}, "standard / hc", "string", "standard"});
	parser.addOption({{"C", "configFile"}, "any valid json file", "string", "config.json"});
	parser.process(application);

	//ConfigManager::fromFile(parser.value("configFile"));

	//	if (parser.value("executionMode") == "hc1") {
	//		executeHc1();
	//		sleep(conf().sleepAfterHcSection);
	//		return EXIT_SUCCESS;
	//	}

	//	init();

	//	if (parser.value("executionMode") == "hc") {
	//		executeHc();
	//		sleep(conf().sleepAfterHcSection);
	//		return EXIT_SUCCESS;
	//	}

	// The io_context is required for all I/O
	//net::io_context httpTrafficIOC{conf().workerLimit};
	net::io_context httpTrafficIOC{5};

	net::io_context monitoringIOC{2}; //we do not really need a lot of power to monitor

	// Create and launch a listening port
	std::make_shared<listener>(
	    httpTrafficIOC,
	    //  tcp::endpoint{net::ip::make_address(conf().listeningAddress), (u_short)conf().listeningPort})
	    tcp::endpoint{net::ip::make_address("127.0.0.1"), 1984})
	    ->run();

	// Create and launch the MONITORING listening port
	std::make_shared<listener>(
	    monitoringIOC,
	    //tcp::endpoint{net::ip::make_address(conf().listeningAddress), (u_short)conf().monitoringPort})
	    tcp::endpoint{net::ip::make_address("127.0.0.1"), 1985})
	    ->run();

	// Capture SIGINT and SIGTERM to perform a clean shutdown
	net::signal_set signals2block(httpTrafficIOC, SIGINT, SIGTERM);
	net::signal_set signals2block2(monitoringIOC, SIGINT, SIGTERM);
	signals2block.async_wait(
	    [&](beast::error_code const&, int) {
		    // Stop the `io_context`. This will cause `run()`
		    // to return immediately, eventually destroying the
		    // `io_context` and all of the sockets in it.
		    httpTrafficIOC.stop();
	    });
	signals2block2.async_wait(
	    [&](beast::error_code const&, int) {
		    // Stop the `io_context`. This will cause `run()`
		    // to return immediately, eventually destroying the
		    // `io_context` and all of the sockets in it.
		    monitoringIOC.stop();
	    });

	//fmt::print("Ready listening on {}:{} and monitoring on {}\n", conf().listeningAddress, conf().listeningPort, conf().monitoringPort);
	fmt::print("Ready listening\n"); // on {}:{} and monitoring on {}\n", conf().listeningAddress, conf().listeningPort, conf().monitoringPort);
	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	//v.reserve(conf().workerLimit + 2);
	//for (auto i = conf().workerLimit; i > 0; --i) {
	for (auto i = 5; i > 0; --i) {
		auto status = threadStatus.newStatus();

		auto& t = v.emplace_back(
		    [&httpTrafficIOC, status] {
			    //I have no idea how to get linux TID (thread id) from the posix one -.- so I have to resort to this
			    status->tid       = gettid();
			    localThreadStatus = status;
			    //and than launch to io handler
			    httpTrafficIOC.run();
		    });

		status->state = ThreadState::Idle;
		status->info  = "just created";

		threadStatus.pool.insert({t.get_id(), status});
	}

	//2 thread are ok for monitoring
	v.emplace_back([&monitoringIOC] { monitoringIOC.run(); });
	monitoringIOC.run();

	// Block until all the threads exit
	for (auto& t : v) {
		t.join();
	}

	return EXIT_SUCCESS;
}
