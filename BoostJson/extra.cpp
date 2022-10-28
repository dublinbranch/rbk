#include "extra.h"
//one off include to compile what is needed and avoid linking external stuff
#include <boost/json/src.hpp>

#include <QByteArray>
#include <QString>

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QString& t) {
	jv = t.toStdString();
}

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QByteArray& t) {
	jv = t.toStdString();
}

/*
https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html

struct customer
{
    std::uint64_t id;
    std::string name;
    bool late;

    customer() = default;

    customer( std::uint64_t i, const std::string& n, bool l )
        : id( i ), name( n ), late( l ) { }


};

void tag_invoke( const bj::value_from_tag&, bj::value& jv, customer const& c )
{
    // Assign a JSON value
    jv = {
        { "id", c.id },
        { "name", c.name },
        { "late", c.late }
    };
}


        std::vector< customer > customers = {
                customer( 0, "Alison", false ),
                customer( 1, "Bill", false ),
                customer( 3, "Catherine", true ),
                customer( 4, "Doug", false )
         };



        bj::value jv = bj::value_from( customers);
                //So it understood was a vector...
                assert( jv.is_array() );

*/

using namespace boost;
void pretty_print(std::string& res, const boost::json::value& jv, std::string indent) {
	switch (jv.kind()) {
	case json::kind::object: {
		res += "{\n";
		indent.append(4, ' ');
		auto const& obj = jv.get_object();
		if (!obj.empty()) {
			auto it = obj.begin();
			for (;;) {
				res += indent + json::serialize(it->key()) + " : ";
				pretty_print(res, it->value(), indent);
				if (++it == obj.end())
					break;
				res += ",\n";
			}
		}
		res += "\n";
		indent.resize(indent.size() - 4);
		res += indent + "}";
		break;
	}

	case json::kind::array: {
		res += "[\n";
		indent.append(4, ' ');
		auto const& arr = jv.get_array();
		if (!arr.empty()) {
			auto it = arr.begin();
			for (;;) {
				res += indent;
				pretty_print(res, *it, indent);
				if (++it == arr.end())
					break;
				res += ",\n";
			}
		}
		res += "\n";
		indent.resize(indent.size() - 4);
		res += indent + "]";
		break;
	}

	case json::kind::string: {
		res += json::serialize(jv.get_string());
		break;
	}

	case json::kind::uint64:
		res += jv.get_uint64();
		break;

	case json::kind::int64:
		res += jv.get_int64();
		break;

	case json::kind::double_:
		res += jv.get_double();
		break;

	case json::kind::bool_:
		if (jv.get_bool())
			res += "true";
		else
			res += "false";
		break;

	case json::kind::null:
		res += "null";
		break;
	}

	if (indent.empty())
		res += "\n";
}
