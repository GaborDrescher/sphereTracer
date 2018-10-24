#include "bbseries.h"

BBSeries::BBSeries(size_t capacity)
{
	this->pos = 0;
	this->cap = capacity;
	this->dataY = new qreal[capacity];

	for(size_t i = 0; i != capacity; ++i) {
		this->dataY[i] = 0;
		this->minMaxY.insert(0);
	}

	this->backwardsMode = false;
}

BBSeries::~BBSeries()
{
	delete [] dataY;
}

void BBSeries::add(qreal y)
{
	qreal oldY = dataY[pos];
	dataY[pos] = y;
	pos = (pos + 1) % cap;

	minMaxY.erase(oldY);
	minMaxY.insert(y);
}

size_t BBSeries::size() const
{
	return cap;
}

QPointF BBSeries::sample(size_t index) const
{
	qreal fcap = (qreal)cap;
	qreal xVal = index;
	if(backwardsMode) {
		xVal = ((-fcap) + 1) + index;
	}
	return QPoint(xVal, dataY[(pos + index) % cap]);
}

QRectF BBSeries::boundingRect() const
{
	qreal xmin = 0;
	qreal xmax = cap-1;
	if(backwardsMode) {
		xmin = -((qreal)(cap - 1));
		xmax = 0;
	}

	qreal ymin = *(minMaxY.cbegin());
	qreal ymax = *(minMaxY.crbegin());

	QRectF out(xmin, ymin, xmax - xmin, ymax - ymin);

	return out;
}

bool BBSeries::getMode()
{
	return backwardsMode;
}

void BBSeries::setMode(bool val)
{
	backwardsMode = val;
}

size_t BBSeries::getCapacity()
{
	return cap;
}

int BBSeries::getMinX()
{
	if(backwardsMode) {
		return -cap;
	}
	else {
		return 0;
	}
}

int BBSeries::getMaxX()
{
	if(backwardsMode) {
		return 0;
	}
	else {
		return cap;
	}
}
