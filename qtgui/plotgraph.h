#ifndef PLOTGRAPH_H
#define PLOTGRAPH_H

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_grid.h>
#include "bbseries.h"

class PlotGraph
{
	public:
	QwtPlot plot;
	QwtPlotCurve curve;
	QwtPlotGrid grid;
	BBSeries *series;

	QwtPlotPanner *panner;
	QwtPlotMagnifier *mag;

	PlotGraph();
	~PlotGraph();
};

#endif // PLOTGRAPH_H
