#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include "EDF/EDFReader.h"

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
};
#endif // MAINWINDOW_H
