#include <boost/test/unit_test.hpp>

#include "rbk/filesystem/filefunction.h"

#include <QTemporaryDir>

namespace {

QByteAdt writeTempFile(QTemporaryDir& dir, const char* name, const QByteArray& bytes) {
	const QString path = dir.path() + QLatin1Char('/') + QLatin1String(name);
	(void)filePutContents(bytes, path, false);
	return QByteAdt(path);
}

} // namespace

BOOST_AUTO_TEST_SUITE(file_get_contents3)

BOOST_AUTO_TEST_CASE(success_exact_revision_and_body_after_marker)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QByteArray fixture = QByteArray(
	    R"({"revision":7}

__HEADER__END__

server{ listen 80; }
)");

	auto path = writeTempFile(dir, "ok.conf", fixture);

	const FGCParam     p{.revision = 7, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(res.exist);
	BOOST_CHECK(res.err == FileGetRes::Err::none);
	BOOST_CHECK(res.content == QByteArray("server{ listen 80; }"));
}

BOOST_AUTO_TEST_CASE(missing_header_end_is_error)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	auto path = writeTempFile(dir, "no_marker.conf", "{\"revision\":7}\nserver{}");

	const FGCParam     p{.revision = 7, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(!res.exist);
	BOOST_CHECK(res.err == FileGetRes::Err::missingHeaderEndMarker);
	BOOST_CHECK(res.content.isEmpty());
}

BOOST_AUTO_TEST_CASE(invalid_json_header)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QByteArray fixture = QByteArray(
	    R"(not json at all

__HEADER__END__

body
)");

	auto path = writeTempFile(dir, "bad_json.conf", fixture);

	const FGCParam     p{.revision = 0, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(!res.exist);
	BOOST_CHECK(res.err == FileGetRes::Err::invalidJsonHeader);
	BOOST_CHECK(res.content.isEmpty());
}

BOOST_AUTO_TEST_CASE(missing_revision_when_revision_required)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QByteArray fixture = QByteArray(
	    R"({})

__HEADER__END__

x
)");

	auto path = writeTempFile(dir, "no_rev.conf", fixture);

	const FGCParam     p{.revision = 1, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(!res.exist);
	BOOST_CHECK(res.err == FileGetRes::Err::missingRevision);
}

BOOST_AUTO_TEST_CASE(revision_mismatch_exact)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QByteArray fixture = QByteArray(
	    R"({"revision":3}

__HEADER__END__

z
)");

	auto path = writeTempFile(dir, "mis.conf", fixture);

	const FGCParam     p{.revision = 7, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(!res.exist);
	BOOST_CHECK(res.err == FileGetRes::Err::revisionMismatch);
}

BOOST_AUTO_TEST_CASE(minimum_satisfied_and_minimum_rejected)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QByteArray fixtureOk = QByteArray(
	    R"({"revision":10})

__HEADER__END__

a
)");
	auto pathOk = writeTempFile(dir, "min_ok.conf", fixtureOk);
	const FGCParam      pOk{.revision = 5, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::minimum};
	const FileGetRes resOk = fileGetContents3(pathOk, pOk);
	BOOST_CHECK(resOk.exist);
	BOOST_CHECK(resOk.content == QByteArray("a"));

	const QByteArray fixtureBad = QByteArray(
	    R"({"revision":4})

__HEADER__END__

b
)");
	auto pathBad = writeTempFile(dir, "min_bad.conf", fixtureBad);
	const FGCParam       pBad{.revision = 5, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::minimum};
	const FileGetRes resBad = fileGetContents3(pathBad, pBad);
	BOOST_CHECK(!resBad.exist);
	BOOST_CHECK(resBad.err == FileGetRes::Err::revisionMismatch);
}

BOOST_AUTO_TEST_CASE(zero_required_revision_skips_revision_check)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QByteArray fixture = QByteArray(
	    R"({"note":"no revision key"})

__HEADER__END__

payload
)");

	auto path = writeTempFile(dir, "skip_rev.conf", fixture);

	const FGCParam     p{.revision = 0, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(res.exist);
	BOOST_CHECK(res.content == QByteArray("payload"));
}

BOOST_AUTO_TEST_CASE(missing_file)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const auto path = QByteAdt(dir.path() + "/does_not_exist.conf");

	const FGCParam     p{.revision = 1, .quiet = true, .maxAge = 0, .mr = FGCParam::MR::exact};
	const FileGetRes res = fileGetContents3(path, p);

	BOOST_CHECK(!res.exist);
	BOOST_CHECK(res.err == FileGetRes::Err::none);
}

BOOST_AUTO_TEST_SUITE_END()
