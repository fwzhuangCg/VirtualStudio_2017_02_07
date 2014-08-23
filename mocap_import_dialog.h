#ifndef MOCAP_IMPORT_DIALOG_H
#define MOCAP_IMPORT_DIALOG_H

#include <QDialog>

#include "ui_mocapimportdialog.h"

class MocapImportDialog : public QDialog, public Ui::MocapImportDialog
{
    Q_OBJECT
public:
    MocapImportDialog(QWidget* parent = 0);

    QString asf_filename;
    QString amc_filename;

signals:
    void mocapSelected(QString& , QString&);

private slots:
    void on_browseASFButton_clicked();
    void on_browseAMCButton_clicked();
    void on_lineEdit_textChanged();
    void on_accepted();
};
#endif // MOCAP_IMPORT_DIALOG_H