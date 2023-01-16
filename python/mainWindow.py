from PyQt5 import QtCore, QtWidgets
from PyQt5.QtCore import pyqtSlot
import pyqtgraph as pg
from pyedflib import highlevel
import numpy as np
from scipy.fft import fft, fftfreq

class MainWindow(QtWidgets.QMainWindow):
    samplingRate = 0
    duration = 0
    path = ''
    channelCount = 0

    def __init__(self, *args: object, **kwargs: object) -> object:
        super(MainWindow, self).__init__(*args, **kwargs)
        self.setupUi()

    def setupUi(self):
        # Создание управляющих элементов в верхнем ряду
        self.labelFileName = QtWidgets.QLabel("")

        btnOpen = QtWidgets.QPushButton("Open")
        btnOpen.clicked.connect(lambda: self.openfile())

        xRange = [ 1, 2, 4, 5, 10, 20, 30, 60, 120, 300 ]
        self.comboXrange = QtWidgets.QComboBox()
        for x in xRange:
            self.comboXrange.addItem(str(x) + " s", x)
        self.comboXrange.setCurrentIndex(xRange.index(10))            
        self.comboXrange.currentIndexChanged.connect(self.updateXrange)

        yRange = [ 1, 10, 20, 25, 50, 100, 200, 500, 1000, 5000, 10000 ]
        self.comboYrange = QtWidgets.QComboBox()
        for y in yRange:
            self.comboYrange.addItem("±" + str(y) + " uV", y)
        self.comboYrange.setCurrentIndex(yRange.index(100))            
        self.comboYrange.currentIndexChanged.connect(self.updateYrange)

        buttonsLayout = QtWidgets.QHBoxLayout()
        buttonsLayout.setContentsMargins(0, 0, 0, 0)
        buttonsLayout.addWidget(btnOpen, 0)
        buttonsLayout.addWidget(self.labelFileName, 100)
        buttonsLayout.addWidget(self.comboXrange, 0)
        buttonsLayout.addWidget(self.comboYrange, 0)

        self.scrollBar = QtWidgets.QScrollBar(QtCore.Qt.Horizontal)
        self.scrollBar.setRange(0, 0)
        self.scrollBar.valueChanged.connect(self.updateXpos)

        # Создание компоновки графиков
        self.graphWidgets = []
        self.spectrumWidgets = []
        self.graphWidgetRows = []
        graphWidgetsHolder = QtWidgets.QWidget()
        graphWidgetsHolder.setContentsMargins(0, 0, 0, 0)
        self.graphWidgetsLayout = QtWidgets.QVBoxLayout(graphWidgetsHolder)
        self.graphWidgetsLayout.setContentsMargins(0, 0, 0, 0)

        # Создание центрального виджета и самого окна          
        centralWidget = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(centralWidget)
        layout.addLayout(buttonsLayout)
        layout.addWidget(graphWidgetsHolder, 100)
        layout.addWidget(self.scrollBar)
        self.setWindowTitle("BioDataReader")  
        self.setCentralWidget(centralWidget)               
        self.showMaximized()   

    def updateXrange(self):
        # Выставление масштаба по горизонтали
        x = int(self.comboXrange.currentData())
        scrollableValue = int(self.duration - x)
        if self.duration > 0:
            self.scrollBar.setRange(0, scrollableValue)
        else:
            self.scrollBar.setRange(0, 0)
        self.updateXpos()

    def updateXpos(self):
        # Обновление позиции просмотра
        x = int(self.comboXrange.currentData())
        start = self.scrollBar.value()
        end = start + x
        for plot in self.graphWidgets:
            plot.setXRange(start, end)

        # Вычисление спектра текущих обозреваемых сигналов 
        sampleStart = int(start * self.samplingRate)
        sampleEnd = int(end * self.samplingRate)

        N = int(x * self.samplingRate)
        T = 1.0 / self.samplingRate
        xf = fftfreq(N, T)[:N//2]
        maxY = 1
        for ch in range(self.channelCount):
            sig = self.signals[ch]
            # Выделение видимой части сигнала
            portion = []
            for i in range(sampleStart, sampleEnd):
                portion.append(sig[i])          

            # Быстрое преобразование Фурье
            yf = fft(portion)

            # Переход от комплексных чисел
            yf2 = 2.0/N * np.abs(yf[0:N//2])
            
            # Вычисление максимальной амплитуды
            maxSpectrum = max(yf2)
            if maxSpectrum > maxY: maxY = maxSpectrum

            # Отрисовка
            self.spectrumWidgets[ch].clear()
            self.spectrumWidgets[ch].plot(xf, yf2)
        
        # Выставление графического максимума
        for ch in range(self.channelCount):
            self.spectrumWidgets[ch].setYRange(0, maxY)
          

    def updateYrange(self):
        # Обновление графических пределов графиков сигналов
        y = self.comboYrange.currentData()
        for plot in self.graphWidgets:
            plot.setYRange(-y, y)

    def openfile(self):
        fileName = QtWidgets.QFileDialog.getOpenFileName(self, "Open EDF", "", "EDF files (*.edf)")[0]
        if len(fileName) == 0: return

        self.path = fileName

        # Очистка старых виджетов
        for widgetRow in self.graphWidgetRows:
            widgetRow.setParent(None)
        self.graphWidgetRows.clear()
        self.spectrumWidgets.clear()
        self.graphWidgets.clear()

        # Чтение данных из заголовка записи
        self.signals, signal_headers, header = highlevel.read_edf(fileName)
        self.samplingRate = signal_headers[0]['sample_rate']
        self.channelCount = len(self.signals)
        self.durationSamples = len(self.signals[0])
        self.duration = self.durationSamples / self.samplingRate

        fileInfo = QtCore.QFileInfo(fileName)
        self.labelFileName.setToolTip(fileName)
        self.labelFileName.setText(fileInfo.fileName() + " (" + str(self.duration) + "s, " + str(self.samplingRate) + "Hz, " + str(self.channelCount) + "ch)")

        self.xStep = 1.0 / self.samplingRate
        for ch in range(self.channelCount):
            # Создания набора (ряда) виджетов для каждого канала
            channelName = signal_headers[ch]['label']
            label = QtWidgets.QLabel(channelName)
            label.setMinimumWidth(32)
            plot = pg.PlotWidget()
            spectrumPlot = pg.PlotWidget()

            widgetRow = QtWidgets.QWidget()
            widgetRowLayout = QtWidgets.QHBoxLayout(widgetRow)
            widgetRowLayout.setContentsMargins(0, 0, 0, 0)
            widgetRowLayout.addWidget(label)
            widgetRowLayout.addWidget(plot, 100)
            widgetRowLayout.addWidget(spectrumPlot, 40)
            
            axisX = []
            axisY = []
            x = 0.0
            
            signal = self.signals[ch] 
            for i in range(self.durationSamples):
                axisX.append(x)
                x += self.xStep
                axisY.append(signal[i])

            plot.plot(axisX, axisY)
            self.graphWidgets.append(plot)
            self.spectrumWidgets.append(spectrumPlot)

            self.graphWidgetRows.append(widgetRow)
            self.graphWidgetsLayout.addWidget(widgetRow)           
        
        # Обновление графических пределов 
        self.updateYrange()
        self.updateXrange()
