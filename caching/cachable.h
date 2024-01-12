#ifndef CACHABLE_H
#define CACHABLE_H

#include <QByteArray>

class Cachable {
      public:
	virtual ~Cachable()                                  = default;
	virtual QByteArray serialize()                       = 0;
	virtual void       deserialize(const QByteArray& in) = 0;
};

#endif // CACHABLE_H
