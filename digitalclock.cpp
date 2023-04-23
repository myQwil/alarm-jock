#include "digitalclock.h"

DigitalClock::DigitalClock(QWidget *parent)
	: QLCDNumber(parent)
{}

void DigitalClock::mousePressEvent(QMouseEvent *) {
	emit pressed();
}
