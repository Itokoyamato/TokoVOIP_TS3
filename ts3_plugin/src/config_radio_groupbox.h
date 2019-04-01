#pragma once

#include <QtWidgets/QGroupBox>

namespace Ui
{
class ConfigRadioGroupBox;
}

class ConfigRadioGroupBox : public QGroupBox
{
    Q_OBJECT
    
public:
    explicit ConfigRadioGroupBox(QWidget *parent = 0);
    ~ConfigRadioGroupBox();

public slots:

    void onInLoValueChanged(double val);
    void onInHiValueChanged(double val);
    void onDestrValueChanged(double val);
    void onRingModFrequencyValueChanged(double val);
    void onRingModMixValueChanged(double val);
    void onOutLoValueChanged(double val);
    void onOutHiValueChanged(double val);

signals:
    void EnabledSet(QString,bool);
    void InLoFreqSet(QString,double);
    void InHiFreqSet(QString,double);
    void DestructionSet(QString,double);
    void RingModFrequencySet(QString,double);
    void RingModMixSet(QString,double);
    void OutLoFreqSet(QString, double);
    void OutHiFreqSet(QString, double);

private:
    Ui::ConfigRadioGroupBox *ui;
};
