#include <boost/test/unit_test.hpp>

#include "rbk/filesystem/filefunction.h"

#include <QTemporaryDir>

#include <csignal>
#include <sys/resource.h>

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

BOOST_AUTO_TEST_SUITE(file_put_contents)

namespace {

// A file size limit for the whole process, with SIGXFSZ ignored: a write past the limit then fails
// with EFBIG. The destructor restores the limit and the signal, also on an exception.
class FileSizeLimit {
      public:
	explicit FileSizeLimit(rlim_t bytes) {
		if (getrlimit(RLIMIT_FSIZE, &before) != 0) {
			return;
		}
		haveBefore = true;
		oldSignal  = std::signal(SIGXFSZ, SIG_IGN);
		rlimit tiny   = before;
		tiny.rlim_cur = bytes;
		ok            = setrlimit(RLIMIT_FSIZE, &tiny) == 0;
	}
	~FileSizeLimit() {
		if (haveBefore) {
			setrlimit(RLIMIT_FSIZE, &before);
			std::signal(SIGXFSZ, oldSignal);
		}
	}
	FileSizeLimit(const FileSizeLimit&)            = delete;
	FileSizeLimit& operator=(const FileSizeLimit&) = delete;

	bool ok = false;

      private:
	rlimit before{};
	bool   haveBefore = false;
	void (*oldSignal)(int) = SIG_DFL;
};

} // namespace

// A file size limit makes the write to disk fail inside QSaveFile::commit() (the bytes wait in the
// write buffer until then). The caller must see the failure, and the old file must stay.
BOOST_AUTO_TEST_CASE(a_commit_that_fails_is_reported_and_keeps_the_old_file)
{
	QTemporaryDir dir;
	BOOST_REQUIRE(dir.isValid());
	const QString path = dir.path() + QStringLiteral("/conf.json");
	BOOST_REQUIRE(filePutContents(QByteArray("old"), path, false).ok);

	// No Boost check while the limit is on: a Boost log to a file could fail in that time.
	FPCRes res{};
	bool   limitSet = false;
	{
		FileSizeLimit limit(10);
		limitSet = limit.ok;
		if (limitSet) {
			res = filePutContents(QByteArray(100, 'n'), path, false);
		}
	}

	BOOST_REQUIRE(limitSet);
	BOOST_CHECK(!res.ok);
	BOOST_CHECK(res.error != QFileDevice::NoError);
	BOOST_CHECK(fileGetContents(path, true) == QByteArray("old"));
}

BOOST_AUTO_TEST_SUITE_END()
