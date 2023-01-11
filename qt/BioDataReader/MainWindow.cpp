#include "MainWindow.h"
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include "EDF/EDFReader.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto central = new QWidget();
    central->setContentsMargins(0,0,0,0);
    auto grid = new QGridLayout(central);
    grid->setContentsMargins(0,0,0,0);
    setCentralWidget(central);
    setMinimumSize(800, 600);

    auto labelFileName = new QLabel();

    auto btnOpen = new QPushButton(tr("Open"));
    connect(btnOpen, &QPushButton::clicked, [=]() {
        auto path = QFileDialog::getOpenFileName(this, tr("EDF file"), "", "EDF (*.edf)");
        if (path.isEmpty()) return;

        m_path = path;
        QFileInfo fileInfo(path);
        labelFileName->setText(fileInfo.fileName());
        labelFileName->setToolTip(fileInfo.absoluteFilePath());

        m_chartView->chart()->removeAllSeries();

        EDFReader reader(path);
        reader.open();
        if (reader.channelAmount() > 0)
        {
            int n = reader.lengthSamples();
            float sampleSize = 1000.0/reader.samplingRate();
            for (int i = 0; i<reader.channelAmount(); i++)
            {
                QString ch = reader.getChannelName(i);
                QLineSeries *series = new QLineSeries();
                series->setName(ch);

                QVector<float> data = reader.getChannelDataFloat(i, 0, n);
                float x = 0;
                for (int j = 0; j<n; j++)
                {
                    series->append(x, data[j]);
                    x += sampleSize;
                }

                m_chartView->chart()->addSeries(series);
            }
        }

        m_chartView->chart()->createDefaultAxes();
    });

    auto topLayout = new QHBoxLayout();
    topLayout->addWidget(btnOpen, 0);
    topLayout->addWidget(labelFileName, 100);

    grid->addLayout(topLayout, 0, 0);
    grid->setRowStretch(1, 100);

//    m_chartView->chart()->addSeries(m_series);

    grid->addWidget(m_chartView, 1, 0);
}

MainWindow::~MainWindow()
{
}

