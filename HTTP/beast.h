#ifndef BEAST_H
#define BEAST_H

#include "beastConfig.h"
#include "rbk/mapExtensor/vectorV2.h"
#include <any>
#include <memory>

namespace std {
class thread;
}
namespace boost {
namespace asio {
class io_context;
}
} // namespace boost

class listener;

struct Beast {
	vectorV2<std::thread*> threads;
	void                   listen();
	void                   listen(const BeastConf& conf);
	BeastConf              conf;

      private:
	void okToRun() const;
	//No idea how to forward declare boost::asio::signal_set
	std::any                  signals2block_p;
	std::shared_ptr<listener> listener_p;
	boost::asio::io_context*  IOC = nullptr;
};

#endif // BEAST_H
