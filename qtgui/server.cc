#include <QApplication>
#include <QBoxLayout>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QPainter>
#include <QFont>
#include <QPen>
#include <QColorDialog>
#include <QtGlobal>
#include <QTimer>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_text.h>
#include <qwt_plot_grid.h>

#include "aspectratiolabel.h"
#include "socketworker.h"
#include "bbseries.h"
#include "plotgraph.h"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

static uint64_t get_ms()
{
	struct timespec tp;
	if(clock_gettime(CLOCK_MONOTONIC_RAW, &tp) != 0) {
		printf("ERROR: clock_gettime()");
		exit(-1);
	}

	return ((uint64_t)tp.tv_sec) * 1000 + ((uint64_t)tp.tv_nsec) / 1000000;
}

static void deleteCleanup(void *ptr)
{
	delete [] (uint8_t*)ptr;
}

class DemoServer : public QObject
{
	public:
	SocketWorker *worker;

	QApplication *app;
	QMainWindow *win;

	AspectRatioLabel *imgContainer;

	QWidget *graphContainer;
	QVBoxLayout *graphLayout;

	QWidget *buttonContainer;
	QHBoxLayout *buttonLayout;

	QWidget *settingsContainer;
	QHBoxLayout *settingsLayout;

	QSplitter *graphSplitter;
	QSplitter *imgSplitter;
	QSplitter *settingsSplitter;

	QPushButton *memOnButton;
	QPushButton *memOffButton;
	QSpinBox *widthBox;
	QSpinBox *rtDepthBox;
	QSpinBox *sppBox;
	QSpinBox *spheresBox;
	QSpinBox *radiusBox;
	QSpinBox *plotThickBox;
	QPushButton *imgFitButton;
	QPushButton *imgColorButton;
	QPushButton *imgScaleButton;
	QSpinBox *labelFontSizeBox;
	QSpinBox *buttonFontSizeBox;

	QPushButton *toggleBackwardsMode;

	QLabel *infoLabel;
	uint64_t lastUpdate;

	QVector<PlotGraph*> graphs;

	QFont buttonFont;
	QFont labelFont;
	QFont imgFont;
	QColor imgFontColor;
	Qt::TransformationMode imgScaleMode;

	void addSpinBox(QBoxLayout *layout, QSpinBox *spinBox, uintptr_t minRange, uintptr_t maxRange, uintptr_t initVal, uintptr_t step, const char *name, void (DemoServer::*handler)(int))
	{
		QLabel *tmpLabel = new QLabel(name);
		layout->addWidget(tmpLabel);
		layout->addWidget(spinBox);
		spinBox->setRange(minRange, maxRange);
		spinBox->setValue(initVal);
		spinBox->setSingleStep(step);
		QObject::connect<void(QSpinBox::*)(int)>(spinBox, &QSpinBox::valueChanged, this, handler);
	}

	void widthChange(int nWidth)
	{
		if((nWidth % 4) != 0) {
			return;
		}
		worker->lock(&worker->guiLock);
			worker->nextWidth = nWidth;
			worker->nextHeight = nWidth;
		worker->unlock(&worker->guiLock);
	}

	void rtDepthChange(int nWidth)
	{
		worker->lock(&worker->guiLock);
			worker->nextRTDepth = nWidth;
		worker->unlock(&worker->guiLock);
	}

	void sppChange(int nWidth)
	{
		worker->lock(&worker->guiLock);
			worker->nextSPP = nWidth;
		worker->unlock(&worker->guiLock);
	}

	void spheresChange(int nWidth)
	{
		worker->lock(&worker->guiLock);
			worker->nextSpheres = nWidth;
		worker->unlock(&worker->guiLock);
	}

	void radiusChange(int nWidth)
	{
		worker->lock(&worker->guiLock);
			worker->nextRadius = nWidth;
		worker->unlock(&worker->guiLock);
	}

	void buttonOn()
	{
		worker->lock(&worker->guiLock);
			worker->nextMemProtActive = 1;
		worker->unlock(&worker->guiLock);
	}

	void buttonOff()
	{
		worker->lock(&worker->guiLock);
			worker->nextMemProtActive = 0;
		worker->unlock(&worker->guiLock);
	}

	void plotThickChange(int val)
	{
		QPen pen;
		pen.setWidthF(val / 10.0);

		for(PlotGraph *g : graphs) {
			g->curve.setPen(pen);
			g->plot.replot();
		}
	}

	void imgScaleButtonPressed()
	{
		if(imgScaleMode == Qt::SmoothTransformation) {
			imgScaleMode = Qt::FastTransformation;
			imgScaleButton->setText("Img Filter: nearest");
		}
		else {
			imgScaleMode = Qt::SmoothTransformation;
			imgScaleButton->setText("Img Filter: linear");
		}
	}

	void imgColorButtonPressed()
	{
		QColor tmp = QColorDialog::getColor(imgFontColor, win, QString("Select Text Color"), QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
		if(tmp.isValid()) {
			imgFontColor = tmp;
		}
	}

	void imgFitButtonPressed()
	{
		int val = imgContainer->height();

		QList<int> widthList;
		widthList.append(val);
		widthList.append(imgSplitter->width() - val);

		imgSplitter->setSizes(widthList);
	}

	void labelFontSizeChange(int val)
	{
		labelFont.setPointSize(val);

		for(PlotGraph *g : graphs) {
			QwtText title = g->plot.title();
			title.setFont(labelFont);
			g->plot.setTitle(title);
			g->plot.replot();
		}
	}

	void buttonFontSizeChange(int val)
	{
		buttonFont.setPointSize(val);
		memOnButton->setFont(buttonFont);
		memOffButton->setFont(buttonFont);
	}

	void toggleBackwardsModePressed()
	{
		for(PlotGraph *g : graphs) {
			g->series->setMode(!g->series->getMode());
			g->plot.setAxisScale(QwtPlot::xBottom, g->series->getMinX(), g->series->getMaxX());
			g->plot.replot();
		}
	}

	void init(SocketWorker *worker, int argc, char *argv[])
	{
		this->imgFontColor = QColor(0xff, 0xff, 0xff, 0xff);
		this->buttonFont = QFont("Helvetica", 20, QFont::Bold);
		this->labelFont = QFont("Helvetica", 20, QFont::Bold);
		this->imgFont = QFont("Helvetica", 12, QFont::Bold);
		this->lastUpdate = 0;
		this->worker = worker;
		app = new QApplication(argc, argv);  
		win = new QMainWindow();

		imgContainer = new AspectRatioLabel();
		imgScaleMode = Qt::FastTransformation;
		QImage img(1024, 1024, QImage::Format_RGB32);
		img.fill(0);
		img = img.scaled(1024, 1024, Qt::IgnoreAspectRatio, imgScaleMode);
		imgContainer->setPixmap(QPixmap::fromImage(img));
		imgContainer->setScaledContents(true);
		imgContainer->setMinimumSize(16, 16);


		graphContainer = new QWidget();
		graphLayout = new QVBoxLayout();
		graphLayout->setSpacing(40);
		graphLayout->setContentsMargins(40,40,40,40);
		graphContainer->setLayout(graphLayout);

		buttonContainer = new QWidget();
		buttonLayout = new QHBoxLayout();
		buttonContainer->setLayout(buttonLayout);

		settingsContainer = new QWidget();
		settingsLayout = new QHBoxLayout();
		settingsContainer->setLayout(settingsLayout);

		graphSplitter = new QSplitter();
		graphSplitter->setOrientation(Qt::Vertical);

		imgSplitter = new QSplitter();

		settingsSplitter = new QSplitter();
		settingsSplitter->setOrientation(Qt::Vertical);

		widthBox = new QSpinBox();
		addSpinBox(settingsLayout, widthBox, 16, 4096, 1024, 4, "Resolution:", &DemoServer::widthChange);

		rtDepthBox = new QSpinBox();
		addSpinBox(settingsLayout, rtDepthBox, 1, 32, 16, 1, "RT Depth:", &DemoServer::rtDepthChange);

		sppBox = new QSpinBox();
		addSpinBox(settingsLayout, sppBox, 1, 4, 1, 3, "SPP:", &DemoServer::sppChange);

		spheresBox = new QSpinBox();
		addSpinBox(settingsLayout, spheresBox, 1, 100, 8, 1, "Spheres:", &DemoServer::spheresChange);

		radiusBox = new QSpinBox();
		addSpinBox(settingsLayout, radiusBox, 1, 200, 30, 1, "Radius:", &DemoServer::radiusChange);

		settingsLayout->addStretch(1);

		toggleBackwardsMode = new QPushButton("Maier mode");
		settingsLayout->addWidget(toggleBackwardsMode);
		QObject::connect<void(QPushButton::*)(void)>(toggleBackwardsMode, &QPushButton::pressed, this, &DemoServer::toggleBackwardsModePressed);

		imgFitButton = new QPushButton("Img fit");
		settingsLayout->addWidget(imgFitButton);
		QObject::connect<void(QPushButton::*)(void)>(imgFitButton, &QPushButton::pressed, this, &DemoServer::imgFitButtonPressed);

		imgColorButton = new QPushButton("Text Color");
		settingsLayout->addWidget(imgColorButton);
		QObject::connect<void(QPushButton::*)(void)>(imgColorButton, &QPushButton::pressed, this, &DemoServer::imgColorButtonPressed);

		imgScaleButton = new QPushButton();
		imgScaleButtonPressed();
		settingsLayout->addWidget(imgScaleButton);
		QObject::connect<void(QPushButton::*)(void)>(imgScaleButton, &QPushButton::pressed, this, &DemoServer::imgScaleButtonPressed);

		plotThickBox = new QSpinBox();
		addSpinBox(settingsLayout, plotThickBox, 0, 200, 20, 1, "Plot thickness:", &DemoServer::plotThickChange);

		labelFontSizeBox = new QSpinBox();
		addSpinBox(settingsLayout, labelFontSizeBox, 5, 50, labelFont.pointSize(), 1, "Label Font Size:", &DemoServer::labelFontSizeChange);

		buttonFontSizeBox = new QSpinBox();
		addSpinBox(settingsLayout, buttonFontSizeBox, 5, 50, buttonFont.pointSize(), 1, "Button Font Size:", &DemoServer::buttonFontSizeChange);

		settingsLayout->addStretch(1);

		infoLabel = new QLabel("0 FPS");
		settingsLayout->addWidget(infoLabel);

		memOnButton = new QPushButton("Memory Protection\nON");
		memOnButton->setFont(buttonFont);
		memOnButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		memOffButton = new QPushButton("Memory Protection\nOFF");
		memOffButton->setFont(buttonFont);
		memOffButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QObject::connect<void(QPushButton::*)(void)>(memOnButton, &QPushButton::pressed, this, &DemoServer::buttonOn);
		QObject::connect<void(QPushButton::*)(void)>(memOffButton, &QPushButton::pressed, this, &DemoServer::buttonOff);
		
		graphs.append(new PlotGraph());
		graphs.append(new PlotGraph());
		graphs[0]->plot.setAxisScale(QwtPlot::xBottom, graphs[0]->series->getMinX(), graphs[0]->series->getMaxX());
		graphs[0]->plot.setAxisScale(QwtPlot::yLeft, 0, 150);
		graphs[0]->plot.setTitle("Total Render Time");
		graphs[0]->plot.setAxisTitle(QwtPlot::yLeft, "ms");
		QwtText title = graphs[0]->plot.title();
		title.setFont(labelFont);
		graphs[0]->plot.setTitle(title);

		graphs[1]->plot.setAxisScale(QwtPlot::xBottom, graphs[1]->series->getMinX(), graphs[1]->series->getMaxX());
		graphs[1]->plot.setAxisScale(QwtPlot::yLeft, 0, 700);
		graphs[1]->plot.setTitle("Render Time: Line 0");
		graphs[1]->plot.setAxisTitle(QwtPlot::yLeft, "ns");
		title = graphs[1]->plot.title();
		title.setFont(labelFont);
		graphs[1]->plot.setTitle(title);


		settingsSplitter->addWidget(imgSplitter);
		settingsSplitter->addWidget(settingsContainer);

		imgSplitter->addWidget(imgContainer);
		imgSplitter->addWidget(graphSplitter);

		graphSplitter->addWidget(graphContainer);
		graphSplitter->addWidget(buttonContainer);

		win->setCentralWidget(settingsSplitter);

		for(PlotGraph *g : graphs) {
			graphLayout->addWidget(&g->plot);
		}

		buttonLayout->addWidget(memOnButton);
		buttonLayout->addWidget(memOffButton);

		imgSplitter->setCollapsible(0, false);
		imgSplitter->setCollapsible(1, false);

		graphSplitter->setCollapsible(0, false);
		graphSplitter->setCollapsible(1, false);

		settingsSplitter->setCollapsible(0, false);
		settingsSplitter->setCollapsible(1, true);

		win->showFullScreen();

		// refesh splitter sizes after "show()"
		this->imgFitButtonPressed();
	}

	void timerTick()
	{
		uint64_t updateTime = get_ms();

		uint64_t fpsTime = updateTime - lastUpdate;
		if(fpsTime != 0) {
			uintptr_t fps = 1000/fpsTime;
			infoLabel->setText(QString("%1 FPS").arg(fps));
		}
		lastUpdate = updateTime;

		ImgData *data = nullptr;
		worker->lock(&worker->dataLock);
			if(!worker->imgDeque.isEmpty()) {
				data = worker->imgDeque.takeFirst();
			}
		worker->unlock(&worker->dataLock);

		if(data != nullptr) {
			bool memProtActive;
			worker->lock(&worker->guiLock);
				memProtActive = worker->nextMemProtActive == 1;
			worker->unlock(&worker->guiLock);

			worker->signal();
			QImage imageObject(data->drawImg, data->width, data->height, QImage::Format_RGB32, deleteCleanup, data->drawImg);

			QImage adjustedImg = imageObject.mirrored().scaled(imgContainer->size(), Qt::KeepAspectRatio, imgScaleMode);
			QPainter painter(&adjustedImg);
			int fontSize = adjustedImg.height() / 40;
			if(fontSize <= 0) {
				fontSize = 1;
			}
			imgFont.setPointSize(fontSize);
			painter.setPen(imgFontColor);
			painter.setFont(imgFont);

			QString protectText;
			if(memProtActive) {
				protectText = QString("Memory Protection ON");
			}
			else {
				protectText = QString("Memory Protection OFF");
			}
			QRect rect = adjustedImg.rect();
			rect.adjust(0, 0, -fontSize, -(fontSize/2));
			painter.drawText(rect, Qt::AlignBottom | Qt::AlignRight, protectText);

			imgContainer->setPixmap(QPixmap::fromImage(adjustedImg));

			graphs[0]->series->add(data->measurements[0] / 1000000);
			graphs[1]->series->add(data->measurements[1]);
			graphs[0]->plot.replot();
			graphs[1]->plot.replot();

			delete data;
		}
		updateTime = get_ms() - updateTime;

		uintptr_t singleShotDelay = 0;
		if(updateTime < 30) {
			singleShotDelay = 30 - updateTime;
		}
		QTimer::singleShot(singleShotDelay, this, &DemoServer::timerTick);
	}

	void run()
	{
		QTimer::singleShot(1000, this, &DemoServer::timerTick);
		app->exec();
	}
};

int main(int argc, char *argv[])
{
	SocketWorker worker;
	worker.init();

	DemoServer server;
	server.init(&worker, argc, argv);

	worker.run();
	server.run();

	return 0;
}
