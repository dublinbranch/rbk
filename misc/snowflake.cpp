#include "snowflake.h"
#include <QDebug>
#include <math.h>

u64 Snowflake::next() {
	auto t2 = TimespecV2::now();
	if (t2 != lastUse) {
		lastUse = t2;
		seq     = 0;
	} else {
		seq++;
	}

	return toUint();
}

u64 Snowflake::toUint() const {
    u64 v = 0;
	v           = ts << 28;
	v |= (ms & 1023) << 18;
	v |= (srvId & 255) << 10;
	v |= (seq & 1023);
	return v;
}

void Snowflake::fromUint(const u64 v) {
	seq   = (v & seqX.mask) >> seqX.pad;
	srvId = (v & srvIdX.mask) >> srvIdX.pad;
	ms    = (v & msX.mask) >> msX.pad;
	ts    = (v & tsX.mask) >> tsX.pad;
}

bool Snowflake::operator==(const Snowflake& j) const {
	auto a = ts == j.ts;
	auto b = ts == j.ts;
	auto c = ts == j.ts;
	auto d = ts == j.ts;
	return a && b && c && d;
}

void Snowflake::testMe() {

	Snowflake i(1), j(1);
	i.ts    = 12345;
	i.ms    = 100;
	i.srvId = 5;
	i.seq   = 18;
	qDebug() << i.toUint();

	j.fromUint(i.toUint());

	if (i == j) {
		qDebug() << "sono uguali";
	} else {
		qFatal("snowflake diversi !!!");
	}
}
