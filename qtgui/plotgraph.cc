#include "plotgraph.h"

#include <QFrame>

PlotGraph::PlotGraph()
{
	QPen pen;
	pen.setWidthF(2.0);
	curve.setPen(pen);
	curve.setRenderHint(QwtPlotItem::RenderAntialiased, true);

	series = new BBSeries(400);
	curve.setSamples(series);
	curve.attach(&plot);

	plot.setCanvasBackground(QBrush(QColor(255,255,255)));
	((QFrame*)(plot.canvas()))->setFrameStyle(QFrame::Plain | QFrame::NoFrame);

	QPen majorPen;
	majorPen.setBrush(Qt::gray);
	majorPen.setStyle(Qt::DashLine);
	majorPen.setWidth(1);
	grid.setMajorPen(majorPen);

	QPen minorPen;
	minorPen.setBrush(Qt::gray);
	minorPen.setStyle(Qt::DotLine);
	minorPen.setWidth(1);
	grid.setMinorPen(minorPen);

	//grid.enableXMin(true);
	//grid.enableYMin(true);
	grid.attach(&plot);

	panner = new QwtPlotPanner(plot.canvas());
	panner->setAxisEnabled(QwtPlot::yLeft, true);
	panner->setAxisEnabled(QwtPlot::xBottom, false);

	mag = new QwtPlotMagnifier(plot.canvas());
	mag->setAxisEnabled(QwtPlot::yLeft, true);
	mag->setAxisEnabled(QwtPlot::xBottom, false);

	plot.replot();
}

PlotGraph::~PlotGraph()
{
	delete mag;
	delete panner;
}
