#ifndef MYTYPE_H
#define MYTYPE_H

#include "mymaria.h"
#include <QDataStream>

class QDataStream;
class MyType {
      public:
	MyType() = default;
	MyType(const enum_field_types& t);
	enum_field_types type;
	bool             isNumeric() const;
	bool             isFloat() const;

	// Friend declaration for serialization MUST be in the header for reason
	friend QDataStream& operator<<(QDataStream& out, const MyType& myType) {
		// Serialize enum_field_types as its underlying integer type
		out << static_cast<int>(myType.type);
		return out;
	}

	// Friend declaration for deserialization  MUST be in the header for reason
	friend QDataStream& operator>>(QDataStream& in, MyType& myType) {
		// Deserialize enum_field_types as an integer and cast back to enum
		int typeAsInt;
		in >> typeAsInt;
		myType.type = static_cast<enum_field_types>(typeAsInt);
		return in;
	}
};

#endif // MYTYPE_H
