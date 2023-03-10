#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include "EDF/EDFReader.h"
#include <QScrollBar>
#include <QComboBox>
#include "spectrum/SpectrumCalc.h"
#include <QAreaSeries>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QString m_path = "";

    EDFReader *m_reader = nullptr;
    QScrollBar *m_scrollBar = nullptr;

    QList<QChartView*> m_charts;
    QList<QChartView*> m_chartsSpectrum;
    QList<QLineSeries*> m_linesSpectrum;
    QList<QWidget*> m_chartRows;

    QVector<float> m_frequencies;

    QComboBox *m_comboHorizontalScale = nullptr;
    QComboBox *m_comboVerticalScale = nullptr;

    int m_posSeconds = 0;
    int m_lengthSeconds = 0;
    int m_visibleSeconds = 10;
    int m_visibleSamples = 0;
    int m_verticalScaleUV = 200;

    int m_channelAmount = 0;
    float m_samplingRate = 0;
    float m_sampleSizeSeconds = 0;

    void rebuildScroll();
    void showData();

    QList<SpectrumCalc*> m_spectrums;
};
#endif // MAINWINDOW_H
