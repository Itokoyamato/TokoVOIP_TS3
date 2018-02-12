#include "config_radio.h"

#include "core/ts_helpers_qt.h"
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include "core/ts_logging_qt.h"

ConfigRadio::ConfigRadio(QWidget *parent, QString server_name, QString channel_name) :
    QDialog(parent),
    m_title(channel_name)
{
    setAttribute( Qt::WA_DeleteOnClose );
}

ConfigRadio::~ConfigRadio()
{}

void ConfigRadio::UpdateEnabled(QString name, bool val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->setChecked(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateBandpassInLowFrequency(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onInLoValueChanged(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateBandpassInHighFrequency(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onInHiValueChanged(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateDestruction(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onDestrValueChanged(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateRingModFrequency(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onRingModFrequencyValueChanged(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateRingModMix(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onRingModMixValueChanged(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateBandpassOutLowFrequency(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onOutLoValueChanged(val);
    channel_strip->blockSignals(false);
}

void ConfigRadio::UpdateBandpassOutHighFrequency(QString name, double val)
{
    auto channel_strip = GetChannelStrip(name);
    channel_strip->blockSignals(true);
    channel_strip->onOutHiValueChanged(val);
    channel_strip->blockSignals(false);
}

ConfigRadioGroupBox* ConfigRadio::GetChannelStrip(QString name)
{
    if (m_channelstrips.contains(name))
        return m_channelstrips.value(name);

    auto channel_strip = new ConfigRadioGroupBox;
    channel_strip->setObjectName(name);
    channel_strip->setTitle((m_title.isEmpty()) ? name : m_title);

    connect(channel_strip, &ConfigRadioGroupBox::EnabledSet, this, &ConfigRadio::EnabledSet);
    connect(channel_strip, &ConfigRadioGroupBox::InLoFreqSet, this, &ConfigRadio::InLoFreqSet);
    connect(channel_strip, &ConfigRadioGroupBox::InHiFreqSet, this, &ConfigRadio::InHiFreqSet);
    connect(channel_strip, &ConfigRadioGroupBox::DestructionSet, this, &ConfigRadio::DestructionSet);
    connect(channel_strip, &ConfigRadioGroupBox::RingModFrequencySet, this, &ConfigRadio::RingModFrequencySet);
    connect(channel_strip, &ConfigRadioGroupBox::RingModMixSet, this, &ConfigRadio::RingModMixSet);
    connect(channel_strip, &ConfigRadioGroupBox::OutLoFreqSet, this, &ConfigRadio::OutLoFreqSet);
    connect(channel_strip, &ConfigRadioGroupBox::OutHiFreqSet, this, &ConfigRadio::OutHiFreqSet);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    if (m_title.isEmpty())
    {
        if (!layout())
        {
            setWindowTitle("Radio FX");
            setLayout(new QHBoxLayout);
        }

        layout()->addWidget(channel_strip);
    }
    else
    {
        setWindowTitle(" ");
        setLayout(new QVBoxLayout);
        TSLogging::Log(QString("title: %1").arg(m_title), LogLevel_DEVEL);
        layout()->addWidget(channel_strip);
        auto remove_button = new QPushButton;
        QIcon remove_icon(QStringLiteral(":/icons/delete.png"));
        remove_button->setIcon(remove_icon);
        remove_button->setAccessibleName("Delete");
        remove_button->setText("Delete");
        connect(remove_button, &QPushButton::clicked, this, [this] (bool checked)
        {
            Q_UNUSED(checked);
            // since the x button occupies Rejected, our bool means isDelete
            this->done(QDialog::DialogCode::Accepted);
        });
        layout()->addWidget(remove_button);
        connect(this, &ConfigRadio::finished, this, [this, name](int r)
        {
            emit channel_closed(r, name);
        });
    }
    m_channelstrips.insert(name, channel_strip);
    setFixedSize(this->sizeHint());
    return channel_strip;
}
