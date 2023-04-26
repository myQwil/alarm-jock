#ifndef QUIZDIALOG_H
#define QUIZDIALOG_H

#include <QDialog>

namespace Ui { class QuizDialog; }

class QuizDialog : public QDialog {
	Q_OBJECT

public:
	explicit QuizDialog(QWidget *parent = nullptr);
	~QuizDialog();

	static int getProgress(QWidget *parent, int q, int a);

private slots:
	void on_btnNext_pressed();
	void on_btnNext_clicked();
	void on_edtAnswer_returnPressed();

private:
	Ui::QuizDialog *ui;
	int questions;
	int answered;
	int correct;

	void newQuestion();
	void eval();
};

#endif // QUIZDIALOG_H
