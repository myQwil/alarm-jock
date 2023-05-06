#ifndef PWDIALOG_H
#define PWDIALOG_H

#include <QDialog>

namespace Ui { class PwDialog; }

class PwDialog : public QDialog {
	Q_OBJECT

public:
	explicit PwDialog(QWidget *parent = nullptr);
	~PwDialog();

	static bool getPass(QWidget *parent, QString password);

private:
	Ui::PwDialog *ui;
};

#endif // PWDIALOG_H
