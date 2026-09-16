#include <boost/test/unit_test.hpp>

#include "rbk/minMysql/sqlrowv2.h"
#include "rbk/mapExtensor/missingkeyex.h"
#include "rbk/number/intTypes.h"
#include "rbk/QStacker/exceptionv2.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <memory>
#include <string>

namespace {

SqlRowV2 makeRow(std::initializer_list<std::tuple<const char*, std::string, bool>> cells) {
	SqlRowV2 row;
	row.columns = std::make_shared<SqlResV2::TypeMap>();
	uint     i  = 0;
	MyType   mt(enum_field_types::MAX_NO_FIELD_TYPES);
	for (auto&& [key, val, isNull] : cells) {
		row.columns->insert({std::string(key), SqlResV2::Field{mt, i}});
		row.data.push_back(val);
		row.nulls.push_back(isNull);
		++i;
	}
	return row;
}

} // namespace

BOOST_AUTO_TEST_SUITE(sqlrowv2_getif)

BOOST_AUTO_TEST_CASE(rqIf_sql_null_leaves_dest) {
	auto        row = makeRow({{"cmmsToken", "NULL", true}});
	std::string dest{"keep"};
	auto        r = row.rqIf("cmmsToken", dest);
	BOOST_CHECK(!r);
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::null);
	BOOST_CHECK_EQUAL(dest, "keep");
	BOOST_CHECK(row.isNull("cmmsToken"));
}

BOOST_AUTO_TEST_CASE(rqIf_null_as_empty_still_null) {
	auto        row = makeRow({{"value", "", true}});
	std::string dest{"keep"};
	auto        r = row.rqIf("value", dest);
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::null);
	BOOST_CHECK_EQUAL(dest, "keep");
	BOOST_CHECK(row.isNull("value"));
}

BOOST_AUTO_TEST_CASE(rqIf_literal_NULL_string_is_string) {
	auto        row = makeRow({{"title", "NULL", false}});
	std::string dest;
	auto        r = row.rqIf("title", dest);
	BOOST_CHECK(r);
	BOOST_CHECK_EQUAL(dest, "NULL");
	BOOST_CHECK(!row.isNull("title"));
}

BOOST_AUTO_TEST_CASE(rqIf_empty_string) {
	auto        row = makeRow({{"title", "", false}});
	std::string dest{"keep"};
	auto        r = row.rqIf("title", dest);
	BOOST_CHECK(r);
	BOOST_CHECK(dest.empty());

	dest = "keep";
	r    = row.rqIf("title", dest, {.skipEmpty = true});
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::empty);
	BOOST_CHECK_EQUAL(dest, "keep");
}

BOOST_AUTO_TEST_CASE(rqIf_empty_QString_skipEmpty) {
	auto    row = makeRow({{"title", "", false}});
	QString dest{"keep"};
	auto    r = row.rqIf("title", dest, {.skipEmpty = true});
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::empty);
	BOOST_CHECK_EQUAL(dest.toStdString(), "keep");
}

BOOST_AUTO_TEST_CASE(rqIf_zero_u32) {
	auto row  = makeRow({{"jobId", "0", false}});
	u32  dest = 99;
	auto r    = row.rqIf("jobId", dest);
	BOOST_CHECK(r);
	BOOST_CHECK_EQUAL(dest, 0u);

	dest = 99;
	r    = row.rqIf("jobId", dest, {.skip0 = true});
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::zero);
	BOOST_CHECK_EQUAL(dest, 99u);
}

BOOST_AUTO_TEST_CASE(rqIf_u32_seven) {
	auto row  = makeRow({{"jobId", "7", false}});
	u32  dest = 0;
	auto r    = row.rqIf("jobId", dest);
	BOOST_CHECK(r);
	BOOST_CHECK_EQUAL(dest, 7u);
}

BOOST_AUTO_TEST_CASE(rqIf_garbage_u32_throws_dest_unchanged) {
	auto row  = makeRow({{"jobId", "abc", false}});
	u32  dest = 42;
	BOOST_CHECK_THROW(row.rqIf("jobId", dest), ExceptionV2);
	BOOST_CHECK_EQUAL(dest, 42u);
}

BOOST_AUTO_TEST_CASE(missing_key) {
	auto        row = makeRow({{"id", "1", false}});
	std::string dest{"keep"};
	auto        r = row.getIf("nope", dest);
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::notFound);
	BOOST_CHECK_EQUAL(dest, "keep");
	BOOST_CHECK_THROW(row.rqIf("nope", dest), MissingKeyEX);
	BOOST_CHECK_THROW(row.isNull("nope"), MissingKeyEX);
	BOOST_CHECK_EQUAL(dest, "keep");
}

BOOST_AUTO_TEST_CASE(columns_nullptr_getIf_no_crash) {
	SqlRowV2    row;
	std::string dest{"keep"};
	auto        r = row.getIf("cmmsToken", dest);
	BOOST_CHECK(r.reason == SqlRowV2::GetReason::notFound);
	BOOST_CHECK_EQUAL(dest, "keep");
}

BOOST_AUTO_TEST_CASE(serialize_round_trip_nulls) {
	auto       row = makeRow({{"a", "NULL", true}, {"b", "hi", false}});
	QByteArray buf;
	{
		QDataStream out(&buf, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_5_15);
		out << row;
	}
	SqlRowV2 loaded;
	{
		QDataStream in(&buf, QIODevice::ReadOnly);
		in.setVersion(QDataStream::Qt_5_15);
		in >> loaded;
		BOOST_CHECK_EQUAL(in.status(), QDataStream::Ok);
	}
	loaded.columns = row.columns;
	BOOST_CHECK_EQUAL(loaded.nulls.size(), row.nulls.size());
	BOOST_CHECK_EQUAL(bool(loaded.nulls[0]), true);
	BOOST_CHECK_EQUAL(bool(loaded.nulls[1]), false);
	BOOST_CHECK(loaded.isNull("a"));
	BOOST_CHECK(!loaded.isNull("b"));
}

BOOST_AUTO_TEST_CASE(replace_clears_null_flag) {
	auto row = makeRow({{"jobId", "NULL", true}});
	BOOST_CHECK(row.isNull("jobId"));
	row.replace("jobId", "12");
	BOOST_CHECK(!row.isNull("jobId"));
	u32 dest = 0;
	BOOST_CHECK(row.rqIf("jobId", dest));
	BOOST_CHECK_EQUAL(dest, 12u);
}

BOOST_AUTO_TEST_SUITE_END()
