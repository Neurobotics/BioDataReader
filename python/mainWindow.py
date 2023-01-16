from PyQt5 import QtCore, QtWidgets, QtGui
from PyQt5.QtWidgets import QMessageBox
from PyQt5.QtCore import pyqtSlot
import pyqtgraph as pg
from pyqtgraph import PlotWidget, plot
from pyedflib import highlevel

class MainWindow(QtWidgets.QMainWindow):
    samplingRate = 0
    duration = 0
    path = ''

    def __init__(self, *args: object, **kwargs: object) -> object:
        super(MainWindow, self).__init__(*args, **kwargs)
        self.setupUi()

    def closeEvent(self, event):
        print("Close")

    def setupUi(self):
        # Создание управляющих элементов в верхнем ряду
        self.labelFileName = QtWidgets.QLabel("")

        btnOpen = QtWidgets.QPushButton("Open")
        btnOpen.clicked.connect(lambda: self.openfile())

        buttonsLayout = QtWidgets.QHBoxLayout()
        buttonsLayout.addWidget(btnOpen, 0)
        buttonsLayout.addWidget(self.labelFileName, 100)

        # Создание компоновки графиков
        self.graphWidgets = []
        self.graphWidgetRows = []
        graphWidgetsHolder = QtWidgets.QWidget()
        self.graphWidgetsLayout = QtWidgets.QVBoxLayout(graphWidgetsHolder)

        # Создание центрального виджета и самого окна          
        centralWidget = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(centralWidget)
        layout.addLayout(buttonsLayout)
        layout.addWidget(graphWidgetsHolder, 100)
        self.setWindowTitle("BioDataReader")  
        self.setCentralWidget(centralWidget)               
        self.showMaximized()            

    @pyqtSlot()
    def openfile(self):
        fileName = QtWidgets.QFileDialog.getOpenFileName(self, "Open EDF", "", "EDF files (*.edf)")[0]
        if len(fileName) == 0: return

        self.path = fileName

        for widgetRow in self.graphWidgetRows:
            widgetRow.setParent(None)
        self.graphWidgetRows.clear()

        fileInfo = QtCore.QFileInfo(fileName)
        self.labelFileName.setToolTip(fileName)
        self.labelFileName.setText(fileInfo.fileName())

        signals, signal_headers, header = highlevel.read_edf(fileName)
        self.samplingRate = signal_headers[0]['sample_rate']
        self.durationSamples = len(signals[0])
        self.duration = self.durationSamples / self.samplingRate
        print(self.duration)
        print(self.durationSamples)
        print(self.samplingRate)
        count = 0
        for header in signal_headers:
            ch = header['label']
            label = QtWidgets.QLabel(ch)
            label.setMinimumWidth(32)
            plot = pg.PlotWidget()
            plot.setYRange(-100.0, 100.0)

            widgetRow = QtWidgets.QWidget()
            widgetRowLayout = QtWidgets.QHBoxLayout(widgetRow)
            widgetRowLayout.addWidget(label)
            widgetRowLayout.addWidget(plot, 100)
            
            axisX = []
            axisY = []
            x = 0.0
            xStep = 1.0 / self.samplingRate
            signal = signals[count] 
            for i in range(self.durationSamples):
                axisX.append(x)
                x += xStep
                axisY.append(signal[i])

            plot.plot(axisX, axisY)

            self.graphWidgetRows.append(widgetRow)
            self.graphWidgetsLayout.addWidget(widgetRow)
            
            count += 1
