#include "MainWindow.h"
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto central = new QWidget();
    central->setContentsMargins(0,0,0,0);
    auto grid = new QGridLayout(central);
    grid->setContentsMargins(0,0,0,0);
    setCentralWidget(central);
    resize(QSize(800, 600));

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
            m_spectrums.clear();

            m_path = path;
            QFileInfo fileInfo(path);
            labelFileName->setText(fileInfo.fileName());
            labelFileName->setToolTip(fileInfo.absoluteFilePath());

            m_chartView->chart()->removeAllSeries();
            m_signals.clear();

            m_channelAmount = m_reader->channelAmount();
            m_samplingRate = m_reader->samplingRate();
            m_lengthSeconds = m_reader->lengthSamples() / m_samplingRate;
            m_sampleSizeSeconds = 1.0 / m_samplingRate;

            int n = m_reader->lengthSamples();

            for (int i = 0; i<m_channelAmount; i++)
            {
                QString ch = m_reader->getChannelName(i);
                QLineSeries *series = new QLineSeries();

                series->setName(ch);

                QVector<float> data = m_reader->getChannelDataFloat(i, 0, n);
                float x = 0;
                for (int j = 0; j < n; j++)
                {
                    series->append(x, data[j]);
                    x += m_sampleSizeSeconds;
                }

                m_signals << series;
                m_chartView->chart()->addSeries(series);

                auto spectrumCalc = new SpectrumCalc();
                spectrumCalc->setChannelName(ch);
                spectrumCalc->setDataFrequency(m_samplingRate);
                spectrumCalc->setWindowSize(4);
                m_spectrums << spectrumCalc;
            }
        }
        else
        {
            m_reader->deleteLater();
            m_reader = nullptr;
        }

        m_chartView->chart()->createDefaultAxes();
        rebuildScroll();
    });

    QList<int> secondsList = { 1, 2, 5, 10, 20, 30, 60, 120, 300 };
    m_comboHorizontalScale = new QComboBox();
    int selectedSecond = -1;
    foreach (auto sec, secondsList)
    {
        m_comboHorizontalScale->addItem(QString::number(sec) + tr(" s"), sec);
        if (sec == m_visibleSeconds)
        {
            selectedSecond = m_comboHorizontalScale->count() - 1;
        }
    }

    m_comboHorizontalScale->setCurrentIndex(selectedSecond);

    connect(m_comboHorizontalScale, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]()
    {
        m_visibleSeconds = m_comboHorizontalScale->currentData().toInt();
        rebuildScroll();
    });


    auto topLayout = new QHBoxLayout();
    topLayout->addWidget(btnOpen, 0);
    topLayout->addWidget(labelFileName, 100);
    topLayout->addWidget(m_comboHorizontalScale, 0);
    topLayout->setContentsMargins(8,8,8,0);

    m_scrollBar = new QScrollBar(Qt::Horizontal);
    m_scrollBar->setRange(0, 0);
    connect(m_scrollBar, &QScrollBar::valueChanged, [=](int value) {
       m_posSeconds = value;
       showData();
    });

    grid->addLayout(topLayout, 0, 0);
    grid->addWidget(m_chartView, 1, 0);
    grid->addWidget(m_scrollBar, 2, 0);
    grid->setRowStretch(1, 100);
}

MainWindow::~MainWindow()
{
}

void MainWindow::rebuildScroll()
{
    m_visibleSamples = m_visibleSeconds * m_samplingRate;

    m_scrollBar->blockSignals(true);
    m_scrollBar->setRange(0, m_lengthSeconds - m_visibleSeconds);
    m_scrollBar->blockSignals(false);

    showData();
}

void MainWindow::showData()
{
    if (!m_chartView || !m_reader || m_lengthSeconds == 0 || m_channelAmount == 0 || m_samplingRate == 0) return;
    m_chartView->chart()->axes(Qt::Horizontal).first()->setRange(m_posSeconds, m_posSeconds + m_visibleSeconds);

    auto data = m_reader->getChannelDataFloat(0, m_posSeconds * m_samplingRate, 4 * m_samplingRate);
    auto spectrum = m_spectrums[0]->calculateSpectrum(data);

    qDebug() << "S" << spectrum;
}

