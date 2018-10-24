#ifndef BBSERIES_H
#define BBSERIES_H

#include <stddef.h>
#include <QtGlobal> // for qreal
#include <QPointF>
#include <QRectF>
#include <qwt_series_data.h>
#include <set>

class BBSeries: public QwtSeriesData<QPointF>
{
	private:
	size_t pos;
	size_t cap;
	qreal *dataY;
	std::multiset<qreal> minMaxY;

	bool backwardsMode;

	public:
	BBSeries(size_t capacity);
	virtual ~BBSeries();
	void add(qreal y);
	virtual size_t size() const;
	virtual QPointF sample(size_t index) const;
	virtual QRectF boundingRect() const;
	bool getMode();
	void setMode(bool val);
	size_t getCapacity();
	int getMinX();
	int getMaxX();
};

#endif // BBSERIES_H
