#ifndef ASPECTRATIOLABEL_H
#define ASPECTRATIOLABEL_H

#include <QLabel>
#include <QtGlobal>

class QPixmap;
class QResizeEvent;

class AspectRatioLabel : public QLabel
{
	private:
	int pixmapWidth = 0;
	int pixmapHeight = 0;

	void updateMargins();

	public:
	AspectRatioLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()) : QLabel(parent, f)
	{
	}

	virtual ~AspectRatioLabel()
	{
	}

	void setPixmap(const QPixmap& pm);

	protected:
	void resizeEvent(QResizeEvent* event) override;
};

#endif // ASPECTRATIOLABEL_H
