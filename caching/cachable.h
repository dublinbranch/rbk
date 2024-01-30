#ifndef CACHABLE_H
#define CACHABLE_H

#include <QByteArray>

class Cachable {
      public:
	virtual ~Cachable()                                  = default;
	virtual QByteArray serialize()                       = 0;
	virtual void       deserialize(const QByteArray& in) = 0;
};

/** Usage example
class CTest : public Cachable{
public:
        QByteArray serialize() override;
        void       deserialize(const QByteArray& in) override;

        QByteArray name;
        int         age;


}

#include <QDataStream>

QByteArray CTest::serialize() {
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    //check if is ok to serialize ?

    out << name;
    out << age;

        return buffer;
}

void CTest::deserialize(const QByteArray& in) {
    QDataStream in(in);
    in >> name;
    in >> age;
}
*/

#endif // CACHABLE_H
