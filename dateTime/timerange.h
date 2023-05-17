#ifndef TIMERANGE_H
#define TIMERANGE_H

#include <QDateTime>
#include <QString>

//Return the value closer to the central one
qint64 closer(qint64 a, qint64 center, qint64 b);

namespace Agg {
const QString min = " min(%1) ";
const QString max = " max(%1) ";

} // namespace Agg

struct Compare {
	enum class Operation : uint {
		EQ  = 0,       // =
		GTE = 1 << 2,  // >=
		LTE = 1 << 4,  // <=
		LT  = 1 << 8,  // <
		GT  = 1 << 16, // >
	};
	enum class MinMax : uint {
		min,
		max,
		NA
	};
	//as of 9 / M12 / 2019 the adwords rescan time is 30minute
	qint64 rangeUp   = 2700; //45 minutes
	qint64 rangeDown = 2700;

	Compare() = default;

	MinMax    minMax    = Compare::MinMax::NA;
	Operation operation = Compare::Operation::EQ;
	QString   getOperator() const;
	QString   getMinMax(const QString& def, const QString& col) const;
};

//TODO Name is bad, rename
struct TimeMargin {
	QDateTime time;
	Compare   range;
	TimeMargin() = default;
	TimeMargin(const QDateTime& _time, bool clamp = true);
	qint64 getCenter() const;
	qint64 getMax() const;
	qint64 getMin(qint64 range2use = 0) const;
	void   clamp(bool clampToDay = true);
};

//TODO Name is bad, rename
struct TimeRange {
	TimeRange();
	TimeRange(QDateTime min, QDateTime max, bool clampToDay = true);
	//Check if one of the two
	TimeMargin from;
	TimeMargin to;

	bool total = false;
};

TimeRange getAlways();
#endif // TIMERANGE_H
