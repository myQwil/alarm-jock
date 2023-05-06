#include "pwdialog.h"
#include "ui_pwdialog.h"

PwDialog::PwDialog(QWidget *parent) : QDialog(parent)
	, ui(new Ui::PwDialog)
{
	ui->setupUi(this);
}

PwDialog::~PwDialog() {
	delete ui;
}

bool PwDialog::getPass(QWidget *parent, QString password) {
	PwDialog dialog(parent);
	dialog.ui->edtPassword->setPlaceholderText(password);
	const int ret = dialog.exec();
	return (!!ret && dialog.ui->edtPassword->text() == password);
}
