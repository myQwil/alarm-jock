#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "quizdialog.h"
#include "pwdialog.h"

#include <QStandardPaths>
#include <QInputDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTime>

enum {
	  freq = 48000
	, samples = 1024
	, channels = 2
};

const static char *devPref[] = {
	  "Built-in Audio Analog Stereo"
	, "GK104 HDMI Audio Controller Digital Stereo (HDMI)"
};

const static int exponent =
	log( channels * sizeof(float) * pd::PdBase::blockSize() ) / M_LN2;

static inline void fail(const char *err) {
	std::cerr << err << std::endl;
	QApplication::quit();
}

static void callback(void *lpd, Uint8 *stream, int size) {
	if (size >= 512) {
		((pd::PdBase *)lpd)->processFloat(size >> exponent, nullptr, (float *)stream);
	}
}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, dev(0)
	, state(Alarm::Off)
	, snooze(0)
	, questions(0)
	, answered(0)
{
	QStringList appdata =
		QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
	std::string path = appdata[0].toStdString();
	std::string file = path + "/main.pd";

	// read settings
	int alarm = -1;
	float vol = 0.35;
	FILE *conf = fopen((path + "/config").c_str(), "r");
	if (conf) {
		char line[1000];
		while (fgets(line, 1000, conf) != NULL) {
			if (line[0] == '#') {
				continue;
			} else if (!strncmp(line, "volume", 6)) {
				vol = fabsf(strtof(line + 7, NULL));
			} else if (!strncmp(line, "alarm", 5)) {
				alarm = abs(strtol(line + 6, NULL, 10));
			} else if (!strncmp(line, "snooze", 6)) {
				snooze = abs(strtol(line + 7, NULL, 10));
			} else if (!strncmp(line, "questions", 9)) {
				questions = abs(strtol(line + 10, NULL, 10));
			} else if (!strncmp(line, "password", 8)) {
				line[strcspn(line, "\r\n")] = '\0';
				password = line + 9;
			}
		}
		fclose(conf);
	}
	if (alarm < 0) {
		QTime now = QTime::currentTime();
		alarm = (now.hour()*3600 + now.minute()*60 + now.second()+3) % 86400;
	}

	// initialize audio

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fail(SDL_GetError());
	}
	if (!lpd.init(0, channels, freq)) {
		SDL_CloseAudio();
		fail("Error initializing pd.");
	}
	lpd.setReceiver(this);

	std::size_t end = file.find_last_of("/\\");
	patch = lpd.openPatch(file.substr(end + 1), file.substr(0, end));
	const std::string dlr = patch.dollarZeroStr();
	dest_tgl  = dlr + "tgl";
	dest_test = dlr + "test";
	lpd.sendFloat(dlr + "vol", vol);

	ui->setupUi(this);
	setState(Alarm::On);

	// set the alarm
	float hr = alarm / 3600.f;
	float mn = fmodf(hr, 1) * 60;
	float sc = fmodf(mn, 1) * 60;
	ui->spnHour->setValue(hr);
	ui->spnMinute->setValue(mn);
	ui->spnSecond->setValue(sc);

	connect(&ticker, &QTimer::timeout, this, &MainWindow::tick);
	connect(&timer, &QTimer::timeout, this, &MainWindow::endSnooze);
	connect(&test, &QTimer::timeout, this, &MainWindow::stopAudio);
	timer.setInterval(snooze * 1000);
	test.setInterval(2000);
	test.setSingleShot(true);
	ticker.start(999);
	tick();
}

MainWindow::~MainWindow() {
	delete ui;
	lpd.closePatch(patch);
	lpd.computeAudio(false);
	SDL_CloseAudioDevice(dev);
	SDL_CloseAudio();
}

void MainWindow::closeEvent(QCloseEvent *e) {
	if (state == Alarm::Active) {
		setState(passwordSuccess() ? (snooze ? Alarm::Snooze : Alarm::Off) : state);
	}
	if (state == Alarm::Snooze) {
		setState(quizSuccess() ? Alarm::Off : state);
	}
	if (state < Alarm::Snooze) {
		e->accept();
	} else {
		e->ignore();
	}
}

void MainWindow::print(const std::string &message) {
	std::cout << message << std::endl;
}

void MainWindow::setState(Alarm s) {
	if (s == state) {
		return;
	}
	QPalette p = ui->lcdClock->palette();
	switch (s) {
	default:
		timer.stop();
		p.setColor(p.WindowText, QColor(128, 128, 128));
		p.setColor(p.Background, QColor(128, 128, 128, 16));
		break;
	case Alarm::On:
		p.setColor(p.WindowText, QColor(000, 255, 255));
		p.setColor(p.Background, QColor(000, 255, 255, 16));
		break;
	case Alarm::Snooze:
		p.setColor(p.WindowText, QColor(255, 215, 000));
		p.setColor(p.Background, QColor(255, 215, 000, 16));
		lpd.sendFloat(dest_tgl, 0);
		stopAudio();
		break;
	case Alarm::Active:
		test.stop();
		p.setColor(p.WindowText, QColor(255, 128, 128));
		p.setColor(p.Background, QColor(255, 128, 128, 16));
		lpd.sendFloat(dest_tgl, 1);
		startAudio();
		break;
	}
	ui->lcdClock->setPalette(p);
	state = s;
}

void MainWindow::startAudio() {
	if (dev) {
		return;
	}
	SDL_AudioSpec spec = {};
	spec.freq = freq;
	spec.format = AUDIO_F32;
	spec.channels = channels;
	spec.samples = samples;
	spec.callback = callback;
	spec.userdata = &lpd;
	for (const char *s : devPref) {
		dev = SDL_OpenAudioDevice(s, 0, &spec, &spec, 0);
		if (dev) {
			break;
		}
	}
	if (!dev) {
		dev = SDL_OpenAudioDevice(NULL, 0, &spec, &spec, 0);
	}
	SDL_PauseAudioDevice(dev, 0);
	lpd.computeAudio(true);
}

void MainWindow::stopAudio() {
	if (!dev) {
		return;
	}
	lpd.computeAudio(false);
	SDL_CloseAudioDevice(dev);
	dev = 0;
}

void MainWindow::endSnooze() {
	setState(Alarm::Active);
}

void MainWindow::tick() {
	QTime now = QTime::currentTime();
	QString text = now.toString("hh:mm:ss");
	ui->lcdClock->display(text);

	if (state == Alarm::On
	 && now.hour()   == ui->spnHour->value()
	 && now.minute() == ui->spnMinute->value()
	 && now.second() == ui->spnSecond->value()) {
		setState(Alarm::Active);
		if (snooze) {
			timer.start();
		}
	}
}

bool MainWindow::passwordSuccess() {
	if (password.isEmpty()) {
		return true;
	}
	return PwDialog::getPass(this, password);
}

bool MainWindow::quizSuccess() {
	if (questions == 0) {
		return true;
	}
	answered = QuizDialog::getProgress(this, questions, answered);
	if (answered >= questions) {
		answered = 0;
		return true;
	} else {
		return false;
	}
}

// Slots

void MainWindow::on_lcdClock_pressed() {
	Alarm s;
	if (state == Alarm::Active) {
		s = passwordSuccess() ? (snooze ? Alarm::Snooze : Alarm::Off) : state;
	} else if (state == Alarm::Snooze) {
		s = quizSuccess() ? Alarm::Off : state;
	} else {
		s = (Alarm)!(int)state;
	}
	setState(s);
}

void MainWindow::on_btnTest_pressed() {
	if (state != Alarm::Active) {
		startAudio();
		test.start();
	}
	lpd.sendBang(dest_test);
}
