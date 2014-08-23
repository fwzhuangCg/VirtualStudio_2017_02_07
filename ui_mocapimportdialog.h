/********************************************************************************
** Form generated from reading UI file 'mocap_import_dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MOCAPIMPORTDIALOG_H
#define UI_MOCAPIMPORTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_MocapImportDialog
{
public:
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;
    QDialogButtonBox *buttonBox;
    QPushButton *browseAMCButton;
    QLineEdit *amcLineEdit;
    QLabel *amcLabel;
    QPushButton *browseASFButton;
    QLineEdit *asfLineEdit;
    QLabel *asfLabel;

    void setupUi(QDialog *MocapImportDialog)
    {
        if (MocapImportDialog->objectName().isEmpty())
            MocapImportDialog->setObjectName(QStringLiteral("MocapImportDialog"));
        MocapImportDialog->setWindowModality(Qt::NonModal);
        MocapImportDialog->resize(450, 150);
        MocapImportDialog->setMaximumSize(QSize(450, 150));
        MocapImportDialog->setModal(true);
        verticalLayout = new QVBoxLayout(MocapImportDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        buttonBox = new QDialogButtonBox(MocapImportDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        buttonBox->setCenterButtons(true);

        gridLayout->addWidget(buttonBox, 2, 0, 1, 3);

        browseAMCButton = new QPushButton(MocapImportDialog);
        browseAMCButton->setObjectName(QStringLiteral("browseAMCButton"));
        browseAMCButton->setToolTipDuration(1000);

        gridLayout->addWidget(browseAMCButton, 1, 2, 1, 1);

        amcLineEdit = new QLineEdit(MocapImportDialog);
        amcLineEdit->setObjectName(QStringLiteral("amcLineEdit"));
        amcLineEdit->setToolTipDuration(1000);

        gridLayout->addWidget(amcLineEdit, 1, 1, 1, 1);

        amcLabel = new QLabel(MocapImportDialog);
        amcLabel->setObjectName(QStringLiteral("amcLabel"));

        gridLayout->addWidget(amcLabel, 1, 0, 1, 1);

        browseASFButton = new QPushButton(MocapImportDialog);
        browseASFButton->setObjectName(QStringLiteral("browseASFButton"));
        browseASFButton->setToolTipDuration(1000);

        gridLayout->addWidget(browseASFButton, 0, 2, 1, 1);

        asfLineEdit = new QLineEdit(MocapImportDialog);
        asfLineEdit->setObjectName(QStringLiteral("asfLineEdit"));
        asfLineEdit->setToolTipDuration(1000);

        gridLayout->addWidget(asfLineEdit, 0, 1, 1, 1);

        asfLabel = new QLabel(MocapImportDialog);
        asfLabel->setObjectName(QStringLiteral("asfLabel"));

        gridLayout->addWidget(asfLabel, 0, 0, 1, 1);


        verticalLayout->addLayout(gridLayout);

#ifndef QT_NO_SHORTCUT
        amcLabel->setBuddy(amcLineEdit);
        asfLabel->setBuddy(asfLineEdit);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(asfLineEdit, browseASFButton);
        QWidget::setTabOrder(browseASFButton, amcLineEdit);
        QWidget::setTabOrder(amcLineEdit, browseAMCButton);
        QWidget::setTabOrder(browseAMCButton, buttonBox);

        retranslateUi(MocapImportDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), MocapImportDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), MocapImportDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(MocapImportDialog);
    } // setupUi

    void retranslateUi(QDialog *MocapImportDialog)
    {
        MocapImportDialog->setWindowTitle(QApplication::translate("MocapImportDialog", "Import mocap", 0));
#ifndef QT_NO_TOOLTIP
        browseAMCButton->setToolTip(QApplication::translate("MocapImportDialog", "Browse Acclaim ASF files", 0));
#endif // QT_NO_TOOLTIP
        browseAMCButton->setText(QApplication::translate("MocapImportDialog", "...", 0));
#ifndef QT_NO_TOOLTIP
        amcLineEdit->setToolTip(QApplication::translate("MocapImportDialog", "Locate the Acclaim AMC file", 0));
#endif // QT_NO_TOOLTIP
        amcLineEdit->setPlaceholderText(QApplication::translate("MocapImportDialog", "Locate AMC file", 0));
        amcLabel->setText(QApplication::translate("MocapImportDialog", "AMC file location:", 0));
#ifndef QT_NO_TOOLTIP
        browseASFButton->setToolTip(QApplication::translate("MocapImportDialog", "Browse Acclaim ASF files", 0));
#endif // QT_NO_TOOLTIP
        browseASFButton->setText(QApplication::translate("MocapImportDialog", "...", 0));
#ifndef QT_NO_TOOLTIP
        asfLineEdit->setToolTip(QApplication::translate("MocapImportDialog", "Locate the Acclaim ASF file", 0));
#endif // QT_NO_TOOLTIP
        asfLineEdit->setPlaceholderText(QApplication::translate("MocapImportDialog", "Locate ASF file", 0));
        asfLabel->setText(QApplication::translate("MocapImportDialog", "ASF file location:", 0));
    } // retranslateUi

};

namespace Ui {
    class MocapImportDialog: public Ui_MocapImportDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MOCAPIMPORTDIALOG_H
