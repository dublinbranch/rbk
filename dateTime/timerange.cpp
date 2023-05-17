#include "timerange.h"
#include "define.h"
#include "util.h"

using namespace std;

qint64 TimeMargin::getMax() const {
	//FIXME fare check per evitare al giorno dopo!
	return time.toSecsSinceEpoch() + range.rangeUp;
}

qint64 TimeMargin::getMin(qint64 range2use) const {
	if (range2use) {
		range2use = std::min(range2use, time.toSecsSinceEpoch() - getMidnight(time).toSecsSinceEpoch());
	} else {
		//FIXME ! evitare di andare al giorno prima ! fare check come di cui sopra
		range2use = range.rangeDown;
	}
	return time.toSecsSinceEpoch() - range2use;
}

TimeMargin::TimeMargin(const QDateTime& _time, bool clamp)
	: time(_time) {
	this->clamp(clamp);
}

qint64 TimeMargin::getCenter() const {
	return time.toSecsSinceEpoch();
}

void TimeMargin::clamp(bool clampToDay) {
	/*
	TODO divertirsi a mettere un paio di test
	controllato solo con
	auto n = QDateTime::currentDateTime();
	auto n0 = getMidnight(n);
	n0.setTime({0,10,0});
	auto n1 = getMidnight(n.addDays(1)).addSecs(-120);
	TimeRange         range = {n0, n1};
	*/
	if (clampToDay) {
		//get the max range of the current day, respecting the timezone!
		range.rangeDown = min(range.rangeDown, time.toSecsSinceEpoch() - getMidnight(time).toSecsSinceEpoch());
		range.rangeUp   = min(range.rangeUp, getMidnight(time.addDays(1)).toSecsSinceEpoch() - time.toSecsSinceEpoch());
	}
}

qint64 closer(qint64 a, qint64 center, qint64 b) {
	auto min = std::abs(center - a);
	auto max = std::abs(center - b);
	if (min > max) {
		return b;
	}
	return a;
}

TimeRange::TimeRange() {
	from.range.operation = Compare::Operation::GTE;
	from.range.minMax    = Compare::MinMax::max;
	to.range.operation   = Compare::Operation::LTE;
	to.range.minMax      = Compare::MinMax::min;
}

TimeRange::TimeRange(QDateTime min, QDateTime max, bool clampToDay) {
	from = {min, clampToDay};
	// fix 1 per airpush free manca nelle progress
	from.range.operation = Compare::Operation::GTE;
	to                   = {max, clampToDay};
	// fix 2 per airpush free manca nelle progress
	to.range.operation = Compare::Operation::LTE;
}

QString Compare::getOperator() const {
	switch (operation) {
	case Operation::EQ:
		return QSL(" = ");
	case Operation::GT:
		return QSL(" > ");
	case Operation::LT:
		return QSL(" < ");
	case Operation::GTE:
		return QSL(" >= ");
	case Operation::LTE:
		return QSL(" <= ");
	}
	throw QSL("this should not happen");
}

QString Compare::getMinMax(const QString& def, const QString& col) const {
	switch (minMax) {
	case MinMax::NA:
		return def.arg(col);
	case MinMax::max:
		return Agg::max.arg(col);
	case MinMax::min:
		return Agg::min.arg(col);
	}
	throw QSL("this should not happen");
}

TimeRange getAlways() {
	TimeRange rangeDelete;
	rangeDelete.from.range.operation = Compare::Operation::GTE;
	rangeDelete.to.range.operation   = Compare::Operation::LTE;
	rangeDelete.from.time            = unixMidnight;
	rangeDelete.to.time              = getMidnight(+1);
	return rangeDelete;
}
