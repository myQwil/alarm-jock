#include "quizdialog.h"
#include "ui_quizdialog.h"
#include <QRandomGenerator>

enum Op {
	  Plus
	, Minus
	, Times
	, Div
};

QuizDialog::QuizDialog(QWidget *parent) : QDialog(parent)
	, ui(new Ui::QuizDialog)
{
	ui->setupUi(this);
}

QuizDialog::~QuizDialog() {
	delete ui;
}

void QuizDialog::newQuestion() {
	ui->edtAnswer->setText("");
	QString s = tr("%1 of %2").arg(answered + 1).arg(questions);
	ui->lblCount->setText(s);
	Op op = (Op)(4 * QRandomGenerator::global()->generateDouble());
	int lo, hi;
	if (op < Op::Times) {
		lo = 4, hi = 24;
	} else {
		lo = 2, hi = 12;
	}
	int a = lo + hi * QRandomGenerator::global()->generateDouble();
	int b = lo + hi * QRandomGenerator::global()->generateDouble();
	char o;
	switch (op) {
	default:        o = '+'; correct = a + b;     break;
	case Op::Minus: o = '-'; correct = a; a += b; break;
	case Op::Times: o = '*'; correct = a * b;     break;
	case Op::Div:   o = '/'; correct = a; a *= b; break;
	}
	ui->lblQuestion->setText(tr("What's %1 %2 %3").arg(a).arg(o).arg(b));
}

void QuizDialog::eval() {
	int answer = strtol(ui->edtAnswer->text().toLocal8Bit(), NULL, 10);
	if (answer == correct) {
		ui->btnNext->setStyleSheet("QPushButton:pressed { background-color: green; }");
		if (++answered >= questions) {
			return done(0);
		}
	} else {
		ui->btnNext->setStyleSheet("QPushButton:pressed { background-color: red; }");
	}
	newQuestion();
}

int QuizDialog::getProgress(QWidget *parent, int q, int a) {
	QuizDialog dialog(parent);
	dialog.questions = q;
	dialog.answered = a;
	dialog.newQuestion();
	dialog.exec();
	return dialog.answered;
}

// Slots

void QuizDialog::on_btnNext_pressed() {
	eval();
}

void QuizDialog::on_btnNext_clicked() {
	ui->edtAnswer->setFocus();
}

void QuizDialog::on_edtAnswer_returnPressed() {
	ui->btnNext->animateClick();
}
