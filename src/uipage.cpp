#include "uipage.h"
#include "module.h"
#include "configwidget.h"
#include "global.h"
#include "skinpage.h"
#include "dummyconfig.h"
#include <QVBoxLayout>
#include <QLabel>
#include <KLocalizedString>
#include <QLineEdit>
#include <fcitxqtinputmethodproxy.h>

Fcitx::UIPage::UIPage(Fcitx::Module* parent) : QWidget(parent)
    ,m_module(parent)
    ,m_layout(new QVBoxLayout(this))
    ,m_label(new QLabel(i18n("Cannot load currently used user interface info"), this))
    ,m_widget(0)
{
    setLayout(m_layout);
    m_layout->addWidget(m_label);
    if (Global::instance()->inputMethodProxy()) {
        QDBusPendingReply< QString > reply = Global::instance()->inputMethodProxy()->GetCurrentUI();
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(getUIFinished(QDBusPendingCallWatcher*)));
    }
}

void Fcitx::UIPage::getUIFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QString> reply(*watcher);
    if (!reply.isValid())
        return;
    QString name = reply.value();
    FcitxAddon* addon = m_module->findAddonByName(name);
    if (addon) {
        FcitxConfigFileDesc* cfdesc = Global::instance()->GetConfigDesc(QString::fromUtf8(addon->name).append(".desc"));
        bool configurable = (bool)(cfdesc != NULL || strlen(addon->subconfig) != 0);
        if (configurable) {
            m_label->hide();
            m_widget = new ConfigWidget(addon, this);
            m_layout->addWidget(m_widget);
            connect(m_widget, SIGNAL(changed()), this, SIGNAL(changed()));
        }
        else {
            m_label->setText(i18n("No configuration options for %1.").arg(QString::fromUtf8(addon->generalname)));
        }

        if (name == "fcitx-classic-ui") {
            do {
                FcitxConfigOption* option = FcitxConfigFileGetOption(m_widget->config()->genericConfig()->configFile, "ClassicUI", "SkinType");
                // this should not happen,  but ,we just protect it
                if (!option)
                    break;

                // this should not happen,  but ,we just protect it
                QLineEdit* lineEdit = static_cast<QLineEdit*>(option->filterArg);
                if (!lineEdit)
                    break;

                m_module->skinPage()->setSkinField(lineEdit);
            } while(0);
        }
    }
}

void Fcitx::UIPage::load()
{
    if (m_widget)
        m_widget->load();
}


void Fcitx::UIPage::save()
{
    if (m_widget)
        m_widget->buttonClicked(QDialogButtonBox::Ok);
}

Fcitx::UIPage::~UIPage()
{

}
