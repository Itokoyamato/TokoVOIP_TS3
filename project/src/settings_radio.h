#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QPointer>

#include "core/ts_context_menu_qt.h"
#include "config_radio.h"
#include "mod_radio.h"
#include "core/ts_infodata_qt.h"

class Plugin_Base;

class SettingsRadio : public QObject, public InfoDataInterface, public ContextMenuInterface
{
    Q_OBJECT
    Q_INTERFACES(InfoDataInterface ContextMenuInterface)

public:
	SettingsRadio(TSServersInfo& servers_info, QObject* parent = nullptr);

    void Init(Radio *radio);
    void shutdown();
    bool onInfoDataChanged(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, uint64 mine, QTextStream &data);

signals:
    void EnabledSet(QString,bool);
    void InLoFreqSet(QString,double);
    void InHiFreqSet(QString,double);
    void DestructionSet(QString,double);
    void RingModFrequencySet(QString,double);
    void RingModMixSet(QString,double);
    void OutLoFreqSet(QString,double);
    void OutHiFreqSet(QString,double);

    void ToggleClientBlacklisted(uint64, anyID);

public slots:
    void onContextMenuEvent(uint64 serverConnectionHandlerID, PluginMenuType type, int menuItemID, uint64 selectedItemID);

private slots:
    void saveSettings(int);
    void on_channel_settings_finished(int r, QString setting_id);

private:
	TSServersInfo& m_servers_info;

    int m_ContextMenuUi = -1;
    int m_ContextMenuChannelUi = -1;
    int m_ContextMenuToggleClientBlacklisted = -1;
    QPointer<ConfigRadio> m_config;

    QPointer<Radio> mP_radio;

    QHash<QPair<uint64,uint64>,QPointer<ConfigRadio> > m_channel_configs;
};
