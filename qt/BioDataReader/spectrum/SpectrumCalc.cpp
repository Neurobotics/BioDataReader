#include "SpectrumCalc.h"
#include <QtMath>

using namespace std;

SpectrumCalc::SpectrumCalc(QObject *parent) : QObject(parent)
{

}

QString SpectrumCalc::channelName()
{
    return m_channelName;
}

void SpectrumCalc::setChannelName(QString name)
{
    m_channelName = name;
}

void SpectrumCalc::setWindowSize(float seconds)
{
    if (seconds < 0.1f) seconds = 0.1f;
    m_windowSizeSeconds = seconds;
    refreshData();
}

void SpectrumCalc::setWindowStep(float seconds)
{
    if (seconds < 0.01f) seconds = 0.01f;
    m_windowStepSeconds = seconds;
    refreshData();
}

void SpectrumCalc::setDataFrequency(float frequency)
{
    if (frequency < 0) frequency = 0;
    m_dataFrequency = frequency;
    refreshData();
}

void SpectrumCalc::setSmoothingWindowPercent(float percent)
{
    if (percent > 50) percent = 50;
    if (percent < 0) percent = 0;
    m_smoothingWindowPercent = percent;
    refreshData();
}

QVector<float> SpectrumCalc::calculateSpectrum(QVector<float> &singleChannelData)
{
    m_lastSpectrum.clear();
    m_lastSum = 0;

    int n = m_windowSizeInt;

    if (singleChannelData.size() != n) return m_lastSpectrum;

    float* pData = singleChannelData.data();
    float datasum = 0;

    for (int i = 0; i < n; i++)
        datasum += *(pData + i);

    float dataMn = datasum/n;
    for (int i = 0; i < n; i++)
        *(pData + i) = *(pData + i) - dataMn;

    double in_smoothingWindowSize = m_smoothingWindowPercent / 100.0;

    if (in_smoothingWindowSize < 0)
        in_smoothingWindowSize = 0;

    if (in_smoothingWindowSize > 0.5f)
        in_smoothingWindowSize = 0.5;

    int L = static_cast<unsigned int>(n* 2 * in_smoothingWindowSize);

    int smoothingType = 1;

    double res = 2*M_PI/L;

    if (L <= n && L > 0)
    {
        if (m_cos_cash.empty() || m_need_recalc_cos_cash)
        {
            m_need_recalc_cos_cash = false;
            switch(smoothingType)
            {
            case 1:
                for (int i = 0; i < L; ++i)
                    m_cos_cash << static_cast<float>(0.5 * (1 - cos(res * i)));
                break;
            case 2:
                for (int i = 0; i < L; ++i)
                    m_cos_cash << static_cast<float>(0.54 - 0.46 * cos(res * i));
            }

            float sum = n - L;
            for (int i = 0; i < L; ++i)
                sum += m_cos_cash[i];

            m_correctionCoef = 1.0*n/sum;
        }

        int len = L/2;
        int sz = n - L;

        for (int i = 0; i < len; ++i)
            (*(pData + i)) *= m_cos_cash[i];

        for (int i = len, k = len + sz; i < L; ++i, ++k)
            (*(pData + k)) *= m_cos_cash[i];

        for (int i = 0; i < n; i++)
            *(pData + i) *= m_correctionCoef;
    }

    QVector<float> out_spectrum;
    QVector<complex<float>> in_intSpec;
    QVector<complex<float>> in_intFFT;

    in_intFFT.clear();
    in_intSpec.clear();

    QVector<float> in_data = QVector<float>(singleChannelData.begin(), singleChannelData.end());

    ConvertDataForFFt(m_nfft, in_data, in_intSpec);

    in_intFFT.resize(in_intSpec.size());

    fft(static_cast<int>(in_intSpec.size()), in_intSpec.data(), in_intFFT.data());

    if (static_cast<int>(out_spectrum.size()) != m_frequencies.size())
        out_spectrum.resize(m_frequencies.size());

    float* pSpectreIn = out_spectrum.data();
    complex<float>* pSpectre = in_intFFT.data();
    for (unsigned int j = 0, sz = m_frequencies.size(); j < sz; j++)
    {
        float rl = (pSpectre + j)->real();
        float img = (pSpectre + j)->imag();
        *(pSpectreIn + j) = 2.0f * sqrt(rl * rl + img * img) / n;
    }

    m_lastSpectrum = QVector<float>(out_spectrum.begin(), out_spectrum.end());
    return m_lastSpectrum;
}

QVector<float> SpectrumCalc::getFrequencies() { return m_frequencies; }

QVector<float> SpectrumCalc::addDataSample(QVector<float> data)
{
    m_data.append(data);

    if (m_data.length() >= m_windowSizeInt && m_windowSizeInt > 0)
    {
        QVector<float> sample = m_data.mid(0, m_windowSizeInt);

        if(sample.size() > 2) m_lastSpectrum = calculateSpectrum(sample);

        emit spectrumCalculated(m_lastSpectrum);

        m_data = m_data.mid(m_windowStepInt);

        return m_lastSpectrum;
    }

    return QVector<float>();
}

QVector<float> SpectrumCalc::getLastSpectrum() { return m_lastSpectrum; }

float SpectrumCalc::getFrequencyStep()
{
    if (m_frequencies.count() > 1) return m_frequencies[1];
    return 0;
}

float SpectrumCalc::getWindowSize() { return m_windowSizeSeconds; }

float SpectrumCalc::getWindowStep() { return m_windowStepSeconds; }

float SpectrumCalc::getSmoothingWindowPercent() { return m_smoothingWindowPercent; }

void SpectrumCalc::refreshData()
{
    m_data.clear();

    m_windowSizeInt = qRound(m_windowSizeSeconds * m_dataFrequency);

    m_windowStepInt = qRound(m_windowStepSeconds * m_dataFrequency);

    m_smoothingWindowSamples = m_windowSizeInt*m_smoothingWindowPercent;

    m_nfft = CalcSpectrumNFFT(m_dataFrequency, m_windowSizeInt);
    m_frequencies = CalcSpectrumFrequencies(m_dataFrequency, m_windowSizeInt);

    m_need_recalc_cos_cash = true;
    m_cos_cash.clear();
}

QVector<float> SpectrumCalc::CalcSpectrumFrequencies(float frequency, int dataSize)
{
    int nfft = CalcSpectrumNFFT(frequency, dataSize);
    float step = CalcSpectrumFrequenciesStepFromNFFT(frequency, nfft);

    QVector<float> fr;
    int n = nfft / 2 + 1;

    for (int i = 0; i<n; i++) fr << i*step;

    return fr;
}

int SpectrumCalc::CalcSpectrumNFFT(float frequency, int dataSize)
{
    Q_UNUSED(frequency)
    int nfft = static_cast<int>(pow(2, ceil(log(dataSize) / log(2))));
    return nfft;
}

float SpectrumCalc::CalcSpectrumFrequenciesStep(float frequency, int dataSize)
{
    float step = (frequency)/CalcSpectrumNFFT(frequency, dataSize);
    return step;
}

float SpectrumCalc::CalcSpectrumFrequenciesStepFromNFFT(float frequency, int nfft)
{
    float step = (frequency)/nfft;
    return step;
}

void SpectrumCalc::ConvertDataForFFt(int in_nfft, QVector<float> &data, QVector<std::complex<float>> &out_vec)
{
    float* pData = data.data();
    const int len = static_cast<int>(data.size());
    out_vec.resize(static_cast<unsigned int>(max(in_nfft, len)));
    complex<float>* pComData = out_vec.data();
    for (int i = 0; i < len; i++)
        (pComData + i)->real(*(pData + i));
}

void SpectrumCalc::fft(int in_sz, std::complex<float> *inout_px, std::complex<float> *in_pData)
{
    if (in_sz == 2)
    {
        complex<float> val1 = *(inout_px) + *(inout_px + 1);
        complex<float> val2 = *(inout_px) - *(inout_px + 1);
        *(inout_px) = val1;
        *(inout_px + 1) = val2;
    }
    else
    {
        unsigned int n = static_cast<unsigned int>(in_sz/2);
        QVector<complex<float>> data(static_cast<unsigned int>(in_sz));
        complex<float>* pXOdd = data.data();
        complex<float>* pXEven = data.data() + n;
        for (unsigned int i = 0; i < n; i++)
        {
            *(pXEven + i) = *(inout_px + 2 * i);
            *(pXOdd + i) = *(inout_px + 2 * i + 1);
        }
        fft(static_cast<int>(n), pXOdd, pXOdd);
        fft(static_cast<int>(n), pXEven, pXEven);
        complex<float> val;
        for (unsigned int i = 0; i < n; i++)
        {
            make_w(static_cast<int>(i), in_sz, val);
            *(in_pData + i) = *(pXEven + i) + val * (*(pXOdd + i));
            *(in_pData + i + n) = *(pXEven + i) - val * (*(pXOdd + i));
        }
    }
}

void SpectrumCalc::make_w(int k, int N, std::complex<float> &out_val)
{
    if (k % N == 0)
    {
        out_val = 1.f;
        return;
    }
    float arg = static_cast<float>(-2 * M_PI * k / N);
    out_val.real(cos(arg));
    out_val.imag(sin(arg));
}
