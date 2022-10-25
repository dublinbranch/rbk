#include "extra.h"

#include <QByteArray>
#include <QString>

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QString& t) {
	jv = {t.toStdString()};
}

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QByteArray& t) {
	jv = {t.toStdString()};
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
