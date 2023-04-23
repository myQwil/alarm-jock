#ifndef DIGITALCLOCK_H
#define DIGITALCLOCK_H

#include <QLCDNumber>

class DigitalClock : public QLCDNumber {
	Q_OBJECT

public:
	DigitalClock(QWidget* = nullptr);

signals:
	void pressed();

protected:
	void mousePressEvent(QMouseEvent*);
};

#endif // DIGITALCLOCK_H
