#include "dsp_radio.h"

#include <QtCore/QVarLengthArray>

#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif
const double TWO_PI_OVER_SAMPLE_RATE = 2*M_PI/48000;

DspRadio::DspRadio(QObject *parent) :
    QObject(parent)
{
    Dsp::Params params;
    params[0] = 48000; // sample rate
    params[1] = 4; // order
    params[2] = 1600; // center frequency
    params[3] = 1300; // band width
    f_m->setParams(params);
    f_s->setParams(params);
    f_m_o->setParams(params);
    f_s_o->setParams(params);
}

void DspRadio::setEnabled(QString name, bool val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (m_enabled == val)
            return;

        m_enabled = val;
    }
}

void DspRadio::setFudge(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (m_fudge == val)
            return;

        m_fudge = val;
        emit fudgeChanged(m_fudge);
    }
}

void DspRadio::setBandpassEqInCenterFrequency(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (f_m)
            f_m->setParam(2, val);
        if (f_s)
            f_s->setParam(2, val);

        emit bandpassEqInCenterFrequencyChanged(val);
    }
}

void DspRadio::setBandpassEqInBandWidth(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (f_m)
            f_m->setParam(3, val);
        if (f_s)
            f_s->setParam(3, val);

        emit bandpassEqInBandWidthChanged(val);
    }
}

void DspRadio::setRmModFreq(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (m_rm_mod_freq == val)
            return;

        m_rm_mod_freq = val;

        emit ringModFrequencyChanged(val);
    }
}

void DspRadio::setRmMix(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (m_rm_mix == val)
            return;

        m_rm_mix = val;

        emit ringModMixChanged(val);
    }
}

void DspRadio::setBandpassEqOutCenterFrequency(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (f_m_o)
            f_m_o->setParam(2, val);
        if (f_s_o)
            f_s_o->setParam(2, val);

        emit bandpassEqOutCenterFrequencyChanged(val);
    }
}

void DspRadio::setBandpassEqOutBandWidth(QString name, double val)
{
    if (name.isEmpty() || (name == m_channel_type))
    {
        if (f_m_o)
            f_m_o->setParam(3, val);
        if (f_s_o)
            f_s_o->setParam(3, val);

        emit bandpassEqOutBandWidthChanged(val);
    }
}

void DspRadio::do_process(float *samples, int sampleCount, float &volFollow)
{
    // ALL INPUTS AND OUTPUTS IN THIS ARE -1.0f and +1.0f
    // Find volume of current block of frames...
    float vol = 0.0f;
 //   float min = 1.0f, max = -1.0f;
    for (int i = 0; i < sampleCount; i++)
    {
       vol += (samples[i] * samples[i]);
    }
    vol /= (float)sampleCount;

    // Fudge factor, inrease for more noise
    vol *= (float)getFudge();

    // Smooth follow from last frame, both multiplies add up to 1...
    volFollow = volFollow * 0.5f + vol * 0.5f;

    // Between -1.0f and 1.0f...
    float random = (((float)(rand()&32767)) / 16384.0f) - 1.0f;

    // Between 1 and 128...
    int count = (rand() & 127) + 1;
    float temp;
    for (int i = 0; i < sampleCount; i++)
    {
       if (!count--)
       {
//          // Between -1.0f and 1.0f...
          random = (((float)(rand()&32767)) / 16384.0f) - 1.0f;
//          // Between 1 and 128...
          count = (rand() & 127) + 1;
       }
        // Add random to inputs * by current volume;
       temp = samples[i] + random * volFollow;

       // Make it an integer between -60 and 60
       temp = (int)(temp * 40.0f);

       // Drop it back down but massively quantised and too high
       temp = (temp / 40.0f);
       temp *= 0.05 * (float)getFudge();
       temp += samples[i] * (1 - (0.05 * (float)getFudge()));
       samples[i] = qBound(-1.0f,temp,1.0f);
    }
}

void DspRadio::do_process_ring_mod(float *samples, int sampleCount, double &modAngle)
{
    if ((m_rm_mod_freq != 0.0f) && (m_rm_mix != 0.0f))
    {
        for (int i=0; i < sampleCount; ++i)
        {
            float sample = samples[i];
            sample = (sample * (1-m_rm_mix)) + (m_rm_mix * (sample * sin(modAngle)));
            samples[i] = qBound(-1.0f,sample,1.0f);
            modAngle += m_rm_mod_freq * TWO_PI_OVER_SAMPLE_RATE;
        }
    }
}

void DspRadio::process(short *samples, int sampleCount, int channels)
{
//    Dsp::Filter* f = (channels==1)?f_m:f_s;

    if (!getEnabled())
        return;

    if (channels == 1)
    {
        QVarLengthArray<float,480> data(sampleCount); //Test has been 480
        for (int i=0; i<sampleCount; ++i)
            data[i] = samples[i] / 32768.f;

        float* audioData[1];
        audioData[0] = data.data();
        f_m->process(sampleCount, audioData);

        do_process_ring_mod(data.data(), sampleCount, m_rm_mod_angle);

        if (getFudge() > 0.0f)
            do_process(data.data(), sampleCount, m_vol_follow);

        f_m_o->process(sampleCount, audioData);

        dsp_volume_agmu.process(samples, sampleCount, channels);

        for(int i=0; i < sampleCount; ++i)
            samples[i] = (short)(data[i] * 32768.f);

    }
    else if (channels == 2)
    {
        // Extract from interleaved and convert to QVarLengthArray<float>
        QVarLengthArray<float,480> c_data_left(sampleCount);
        QVarLengthArray<float,480> c_data_right(sampleCount);
        for(int i=0; i < sampleCount; ++i)
        {
            c_data_left[i] = samples[i*2] / 32768.f;
            c_data_right[i] = samples[1 + (i*2)] / 32768.f;
        }

        float* audioData[2];
        audioData[0] = c_data_left.data();
        audioData[1] = c_data_right.data();
        f_s->process(sampleCount, audioData);

        do_process_ring_mod(c_data_left.data(), sampleCount, m_rm_mod_angle);
        do_process_ring_mod(c_data_right.data(), sampleCount, m_rm_mod_angle_r);

        if (getFudge() > 0.0f)
        {
            do_process(c_data_left.data(), sampleCount, m_vol_follow);
            do_process(c_data_right.data(), sampleCount, m_vol_follow_r);
        }

        f_s_o->process(sampleCount, audioData);

        dsp_volume_agmu.process(samples, sampleCount, channels);

        for(int i=0; i < sampleCount; ++i)
        {
            samples[i*2] = (short)(c_data_left[i] * 32768.f);
            samples[1 + (i*2)] = (short)(c_data_right[i] * 32768.f);
        }
    }
}

void DspRadio::setChannelType(QString name)
{
    m_channel_type = name;
}

float DspRadio::getFudge() const
{
    return m_fudge;
}

double DspRadio::getBandpassEqInCenterFrequency() const
{
    if (f_m)
        return f_m->getParam(2);
    else if (f_s)
        return f_s->getParam(2);
    else
        return -1;
}

double DspRadio::getBandpassEqInBandWidth() const
{
    if (f_m)
        return f_m->getParam(3);
    else if (f_s)
        return f_s->getParam(3);
    else
        return -1;
}

double DspRadio::getRmModFreq() const
{
    return m_rm_mod_freq;
}

double DspRadio::getBandpassEqOutCenterFrequency() const
{
    if (f_m_o)
        return f_m_o->getParam(2);
    else if (f_s_o)
        return f_s_o->getParam(2);
    else
        return -1;
}

double DspRadio::getBandpassEqOutBandWidth() const
{
    if (f_m_o)
        return f_m_o->getParam(3);
    else if (f_s_o)
        return f_s_o->getParam(3);
    else
        return -1;
}
