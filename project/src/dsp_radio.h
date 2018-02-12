#pragma once

#include <QtCore/QObject>
#include <DspFilters/Dsp.h>
#include <memory>
#include "volume/dsp_volume_agmu.h"

class DspRadio : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ getEnabled NOTIFY enabledSet)
    Q_PROPERTY(double fudge READ getFudge NOTIFY fudgeChanged)
    Q_PROPERTY(double eq_bp_center_frequency READ getBandpassEqInCenterFrequency NOTIFY bandpassEqInCenterFrequencyChanged)
    Q_PROPERTY(double eq_bp_band_width READ getBandpassEqInBandWidth NOTIFY bandpassEqInBandWidthChanged)
    Q_PROPERTY(double rm_modFreq READ getRmModFreq NOTIFY ringModFrequencyChanged)

public:
    explicit DspRadio(QObject *parent = 0);
    
    void process(short* samples, int sampleCount, int channels);

    void setChannelType(QString name);

    bool getEnabled() const {return m_enabled;}
    float getFudge() const;
    double getBandpassEqInCenterFrequency() const;
    double getBandpassEqInBandWidth() const;
    double getRmModFreq() const;
    double getBandpassEqOutCenterFrequency() const;
    double getBandpassEqOutBandWidth() const;

signals:
    void enabledSet(bool);
    void fudgeChanged(double);
    void bandpassEqInCenterFrequencyChanged(double);
    void bandpassEqInBandWidthChanged(double);
    void ringModFrequencyChanged(double);
    void ringModMixChanged(double);
    void bandpassEqOutCenterFrequencyChanged(double);
    void bandpassEqOutBandWidthChanged(double);

public slots:
    void setEnabled(QString name, bool val);
    void setFudge(QString name, double val);
    void setBandpassEqInCenterFrequency(QString name, double val);
    void setBandpassEqInBandWidth(QString name, double val);
    void setRmModFreq(QString name, double val);
    void setRmMix(QString name, double val);
    void setBandpassEqOutCenterFrequency(QString name, double val);
    void setBandpassEqOutBandWidth(QString name, double val);

private:
    void do_process(float *samples, int sampleCount, float &volFollow);
    void do_process_ring_mod(float *samples, int sampleCount, double &modAngle);

    QString m_channel_type;

    float m_vol_follow = 0.0f;
    float m_vol_follow_r = 0.0f;

    bool m_enabled;
    double m_fudge = 0.0f;

    std::unique_ptr<Dsp::Filter> f_m = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 1, Dsp::DirectFormII> >(1024);
    std::unique_ptr<Dsp::Filter> f_s = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 2, Dsp::DirectFormII> >(1024);

    std::unique_ptr<Dsp::Filter> f_m_o = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 1, Dsp::DirectFormII> >(1024);
    std::unique_ptr<Dsp::Filter> f_s_o = std::make_unique< Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass <4>, 2, Dsp::DirectFormII> >(1024);

    //RingMod
    double m_rm_mod_freq = 0.0f;
    double m_rm_mod_angle = 0.0f;
    double m_rm_mod_angle_r = 0.0f;
    double m_rm_mix = 0.0f;

    DspVolumeAGMU dsp_volume_agmu;
};
