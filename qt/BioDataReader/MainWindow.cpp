#include "MainWindow.h"
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto central = new QWidget();
    central->setContentsMargins(0,0,0,0);
    auto grid = new QGridLayout(central);
    grid->setContentsMargins(0,0,0,0);
    setCentralWidget(central);
    setMinimumSize(800, 600);

    m_chartView = new QChartView();

    auto labelFileName = new QLabel();

    auto btnOpen = new QPushButton(tr("Open"));
    connect(btnOpen, &QPushButton::clicked, [=]() {
        auto path = QFileDialog::getOpenFileName(this, tr("EDF file"), "", "EDF (*.edf)");
        if (path.isEmpty()) return;

        if (m_reader) {
            m_reader->deleteLater();
            m_reader = nullptr;
        }

        m_reader = new EDFReader(path);
        if (m_reader->open() && m_reader->channelAmount() > 0)
        {
            m_path = path;
            QFileInfo fileInfo(path);
            labelFileName->setText(fileInfo.fileName());
            labelFileName->setToolTip(fileInfo.absoluteFilePath());

            m_chartView->chart()->removeAllSeries();

            int n = m_reader->lengthSamples();
            int channelAmount = m_reader->channelAmount();
            float sampleSize = 1000.0/m_reader->samplingRate();
            for (int i = 0; i<channelAmount; i++)
            {
                QString ch = m_reader->getChannelName(i);
                QLineSeries *series = new QLineSeries();
                series->setName(ch);
                qDebug() << "CH" <<ch;

                QVector<float> data = m_reader->getChannelDataFloat(i, 0, n);
                float x = 0;
                for (int j = 0; j<n; j++)
                {
                    series->append(QPointF(x, data[j]));
                    x += sampleSize;
                }
                m_chartView->chart()->addSeries(series);
            }
        }
        else
        {
            m_reader->deleteLater();
            m_reader = nullptr;
        }

        m_chartView->chart()->createDefaultAxes();
    });

    auto topLayout = new QHBoxLayout();
    topLayout->addWidget(btnOpen, 0);
    topLayout->addWidget(labelFileName, 100);
    topLayout->setContentsMargins(8,8,8,0);

    grid->addLayout(topLayout, 0, 0);
    grid->setRowStretch(1, 100);

//    m_chartView->chart()->addSeries(m_series);

    grid->addWidget(m_chartView, 1, 0);
}

MainWindow::~MainWindow()
{
}

