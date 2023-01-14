#ifndef SPECTRUMCALC_H
#define SPECTRUMCALC_H

#include <QDateTime>
#include <QObject>
#include <QVector>
#include <QDebug>
#include <vector>
#include <complex>
#include <valarray>

class SpectrumCalc : public QObject
{
    Q_OBJECT
public:
    explicit SpectrumCalc(QObject *parent = nullptr);

    QString channelName();

    void setChannelName(QString name);
    void setWindowSize(float seconds);
    void setWindowStep(float seconds);
    void setDataFrequency(float frequency);
    void setSmoothingWindowPercent(float percent);

    QVector<float> calculateSpectrum(QVector<float> &singleChannelData);
    QVector<float> getFrequencies();
    QVector<float> addDataSample(QVector<float> data);
    QVector<float> getLastSpectrum();

    float getFrequencyStep();
    float getWindowSize();
    float getWindowStep();
    float getSmoothingWindowPercent();

signals:
    void spectrumCalculated(QVector<float> values);

protected:
    QString m_channelName = "";

    QVector<float> m_frequencies;
    QVector<float> m_data;
    QVector<float> m_lastSpectrum;

    float m_lastSum = 0;

    void refreshData();

    float m_dataFrequency = 0;
    float m_windowSizeSeconds = 0;
    int m_windowSizeInt = 0;
    float m_windowStepSeconds = 0;
    float m_smoothingWindowPercent = 0;

    int m_channelAmount = 0;

    int m_dataFrequencyInt = 0;

    int m_windowStepInt = 0;
    int m_smoothingWindowSamples = 0;

    double m_correctionCoef = 0;

    int m_nfft = 0;

    bool m_need_recalc_cos_cash = true;
    QVector<double> m_cos_cash;

public:
    static QVector<float> CalcSpectrumFrequencies(float frequency, int dataSize);

    static int CalcSpectrumNFFT(float frequency, int dataSize);

    static float CalcSpectrumFrequenciesStep(float frequency, int dataSize);

    static float CalcSpectrumFrequenciesStepFromNFFT(float frequency, int nfft);

    static void ConvertDataForFFt(int in_nfft, QVector<float>& data, QVector<std::complex<float>>& out_vec);

    static void PerformFFT(int in_sz, std::complex<float>* inout_px, std::complex<float> *in_pData);

    static void fft(QVector<std::complex<float>>& x, QVector<std::complex<float>>& out_vec);

    static void fft(QVector<std::complex<float>>& x);

    static void fft(int in_sz, std::complex<float>* inout_px, std::complex<float> *in_pData);

    static void make_w(int k, int N, std::complex<float>& out_val);

};

#endif // SPECTRUMCALC_H
