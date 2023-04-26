#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <SDL2/SDL.h>
#include <PdBase.hpp>
#include <QTimer>

enum Alarm {
	  Off    = 0
	, On     = 1
	, Snooze = 3
	, Active = 5
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public pd::PdReceiver {
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_lcdClock_pressed();
	void on_btnTest_pressed();

private:
	Ui::MainWindow *ui;
	SDL_AudioDeviceID dev;
	pd::Patch patch;
	pd::PdBase lpd;
	QTimer ticker;
	QTimer timer;
	QTimer test;
	Alarm state;
	int snooze;
	int questions;
	int answered;
	QString password;

	// pd messages
	std::string dest_tgl;
	std::string dest_test;
	void print(const std::string &);

	void closeEvent(QCloseEvent *);
	void setState(Alarm);
	void startAudio();
	void stopAudio();
	void endSnooze();
	void tick();

	bool passwordSuccess();
	bool quizSuccess();
};

#endif // MAINWINDOW_H
