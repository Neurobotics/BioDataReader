#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include "EDF/EDFReader.h"
#include <QScrollBar>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


protected:
    QString m_path = "";
    QChartView *m_chartView = nullptr;
    EDFReader *m_reader = nullptr;
    QScrollBar *m_scrollBar = nullptr;

    QList<QLineSeries*> m_signals;

    int m_posSeconds = 0;
    int m_lengthSeconds = 0;
    int m_visibleSeconds = 5;
    int m_visibleSamples = 0;

    int m_channelAmount = 0;
    float m_samplingRate = 0;
    float m_sampleSizeSeconds = 0;

    void rebuildScroll();
    void showData();
};
#endif // MAINWINDOW_H
