#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardPaths>
#include <QTime>

enum {
	  freq = 48000
	, samples = 1024
	, channels = 2
};

const static char *devName = "Built-in Audio Analog Stereo";

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


MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, dev(0)
	, state(Alarm::Off)
	, snooze(1800)
{
	QStringList appdata =
		QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
	std::string path = appdata[0].toStdString();
	std::string file = path + "/main.pd";

	// read settings
	int alarm = 0;
	float vol = 0.35;
	FILE *conf = fopen((path + "/config").c_str(), "r");
	if (conf) {
		char line[1000];
		while (fgets(line, 1000, conf) != NULL) {
			if (!strncmp(line, "volume", 6)) {
				vol = strtof(line + 7, NULL);
			} else if (!strncmp(line, "alarm", 5)) {
				alarm = strtol(line + 6, NULL, 10);
			} else if (!strncmp(line, "snooze", 6)) {
				snooze = strtol(line + 7, NULL, 10);
			}
		}
		fclose(conf);
	}

	// initialize audio

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fail(SDL_GetError());
	}
	if (!lpd.init(0, channels, freq)) {
		SDL_CloseAudio();
		fail("Error initializing pd.");
	}

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

void MainWindow::setState(Alarm s) {
	if (s == state) {
		return;
	}
	QPalette p = ui->lcdClock->palette();
	switch (s) {
	case Alarm::Off:
		p.setColor(p.WindowText, QColor(128, 128, 128));
		p.setColor(p.Background, QColor(128, 128, 128, 16));
		timer.stop();
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
		p.setColor(p.WindowText, QColor(255, 128, 128));
		p.setColor(p.Background, QColor(255, 128, 128, 16));
		lpd.sendFloat(dest_tgl, 0);
		lpd.sendFloat(dest_tgl, 1);
		startAudio();
		break;
	}
	ui->lcdClock->setPalette(p);
	state = s;
}

void MainWindow::startAudio() {
	SDL_AudioSpec spec = {};
	spec.freq = freq;
	spec.format = AUDIO_F32;
	spec.channels = channels;
	spec.samples = samples;
	spec.callback = callback;
	spec.userdata = &lpd;
	dev = SDL_OpenAudioDevice(devName, 0, &spec, &spec, 0);
	SDL_PauseAudioDevice(dev, 0);
	lpd.computeAudio(true);
}

void MainWindow::stopAudio() {
	lpd.computeAudio(false);
	SDL_CloseAudioDevice(dev);
	dev = 0;
}

void MainWindow::endSnooze() {
	setState(Alarm::Active);
}

void MainWindow::tick() {
	QTime time = QTime::currentTime();
	QString text = time.toString("hh:mm:ss");
	ui->lcdClock->display(text);

	if (state == Alarm::On
	 && time.hour()   == ui->spnHour->value()
	 && time.minute() == ui->spnMinute->value()
	 && time.second() == ui->spnSecond->value()) {
		setState(Alarm::Active);
		timer.start(snooze * 1000);
	}
}

// Slots
void MainWindow::on_lcdClock_pressed() {
	setState(state == Alarm::Active ? Alarm::Snooze : (Alarm)!(int)state);
}

void MainWindow::on_btnTest_pressed() {
	if (state != Alarm::Active) {
		if (!dev) {
			startAudio();
		}
		test.start(2000);
	}
	lpd.sendBang(dest_test);
}
