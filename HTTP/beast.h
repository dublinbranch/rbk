#ifndef BEAST_H
#define BEAST_H

#include "beastConfig.h"

struct Beast {
	void      listen();
	void      listen(const BeastConf& conf_);
	BeastConf conf;

      private:
	void okToRun() const;
};

#endif // BEAST_H
