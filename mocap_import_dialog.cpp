#include "mocap_import_dialog.h"

#include <QFileDialog>

MocapImportDialog::MocapImportDialog(QWidget* parent /*= 0*/)
    : QDialog(parent)
{
    setupUi(this);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(asfLineEdit, SIGNAL(editingFinished()), this, SLOT(on_lineEditTextChanged()));
    connect(amcLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(on_lineEdit_textChanged()));
    connect(this, SIGNAL(accepted()), this, SLOT(on_accepted()));
}

void MocapImportDialog::on_browseASFButton_clicked()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Import ASF"),  ".", tr("Acclaim ASF files (*.asf)"));
    asfLineEdit->setText(str);
}

void MocapImportDialog::on_browseAMCButton_clicked()
{
    QString str = QFileDialog::getOpenFileName(this, tr("Import AMC"),  ".", tr("Acclaim AMC files (*.amc)"));
    amcLineEdit->setText(str);
}

void MocapImportDialog::on_lineEdit_textChanged()
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(QFileInfo(asfLineEdit->text()).exists() &&
        QFileInfo(asfLineEdit->text()).isFile() && 
        QFileInfo(amcLineEdit->text()).exists() && 
        QFileInfo(amcLineEdit->text()).isFile());
    asf_filename = asfLineEdit->text();
    amc_filename = amcLineEdit->text();
}

void MocapImportDialog::on_accepted()
{
    emit mocapSelected(asf_filename, amc_filename);
}
