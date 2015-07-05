#include "settingswindow.h"
#include "ui_settingswindow.h"

#include "snore.h"
#include "snore_p.h"
#include "settingsdialog.h"
#include "utils.h"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>

using namespace Snore;

SettingsWindow::SettingsWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    ui->widget->show();

    QStringList list = knownApps();
    list.removeAll(qApp->applicationName());
    ui->comboBox->addItems(list);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

QStringList SettingsWindow::knownApps()
{
    return allSettingsKeysWithPrefix(Utils::settingsVersionSchema() + QLatin1String("/LocalSettings"), settings(),
    [](QSettings & settings) {
        return settings.childGroups();
    });
}

QSettings &SettingsWindow::settings()
{
    static QSettings settings(QLatin1String("Snorenotify"), QLatin1String("libsnore"));
    return settings;
}

void SettingsWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    SnoreCorePrivate::instance()->setLocalSttingsPrefix(arg1);
    ui->widget->initTabs();
    ui->widget->setVisible(true);
}

void SettingsWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::AcceptRole:
        ui->widget->accept();
        qApp->quit();
        break;
    case QDialogButtonBox::ApplyRole:
        ui->widget->accept();
        break;
    case QDialogButtonBox::ResetRole:
        ui->widget->reset();
        break;
    case QDialogButtonBox::RejectRole:
        qApp->quit();
        break;
    default:
        snoreDebug(SNORE_WARNING) << "unhandled role" << button->text() << ui->buttonBox->buttonRole(button);
    }
}
