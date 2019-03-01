import serial
import json
import sys

# from PyQt5 import QtCore, QtGui, QtWidgets
from ArduinoGuiQt5 import Ui_MainWindow
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import *


# from PyQt5 import QtWidgets
# from PyQt5.QtWidgets import QMainWindow, QApplication
# from PyQt5 import uic

# Ui_MainWindow, QtBaseClass = uic.loadUiType('Arduino.ui')


# class MyApp(QMainWindow):
# class myMainWindow(QtWidgets.QMainWindow, Ui_MainWindow):
class myMainWindow(Ui_MainWindow):
    def __init__(self, parent=None):
        super(myMainWindow, self).__init__(parent)
        self.setupUi(self)

    def updateRun(self):
        ser = serial.Serial('/dev/ttyACM0', 9600)
        ktieto, ylilampo, ylipitkas, lamporel, kayntisallittu = 0, 0, 0, 0, 0
        self.savukaasu.setText('Kek ')
        print("kakka")
        while True:
            try:
                readings = ser.readline()  # type: object
                if len(readings) > 190:
                    print(readings)
                    fixReadings = readings.decode("utf-8")
                    print(fixReadings)
                    readingsJson = json.loads(fixReadings)
                    if 'Savukaasu' in fixReadings:
                        self.savukaasu.setText(readingsJson[savukaasu])
                    if 'Tulipesa' in fixReadings:
                        self.tulipesa.setText(readingsJson[Tulipesa])
                    if 'Lvesi' in fixReadings:
                        self.lVesi.setText(readingsJson[Lvesi])
                    if 'Pvesi' in fixReadings:
                        self.pVesi.setText(readingsJson[Pvesi])
                    if 'SahlVesi' in fixReadings:
                        self.sahLVesi.setText(readingsJson[SahlVesi])
                    if 'SahPVesi' in fixReadings:
                        self.sahPVesi.setText(readingsJson[SahPVesi])
                    if 'Varaus' in fixReadings:
                        self.varaus.setText(readingsJson[Varaus])
                    if 'Khuone' in fixReadings:
                        self.khuone.setText(readingsJson[Khuone])
                    if 'kayntitieto' in fixReadings:
                        ktieto = readingsJson[kayntitieto]
                    if 'ylilampo' in fixReadings:
                        ylilampo = readingsJson[ylilampo]
                    if 'ylipitkasyotto' in fixReadings:
                        ylipitkas = readingsJson[ylipitkasyotto]
                    if 'Lamporelel' in fixReadings:
                        lamprel = readingsJson[Lamporelel]
                    if 'kayntisallittu' in fixReadings:
                        kayntisallittu = readingsJson[kayntisallittu]

                    if ktieto == 1:
                        self.ui.apoltin.setText("KYLLÄ")
                        self.ui.poltin.setStyleSheet('''
                                            MyLineClass {
                                                color:Green
                                            }
                                        ''')
                    else:
                        self.ui.apoltin.setText("EI")
                        self.ui.poltin.setStyleSheet('''
                                            MyLineClass {
                                                color:Green
                                            }
                                        ''')
                else:
                    print("Heck")
            except:
                print("Problems with decoding JSON")


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    MainWindow = QtWidgets.QMainWindow()
    # mainUpdate()
    ui = Ui_MainWindow()
    ui.setupUi(MainWindow)
    MainWindow.show()
    ex = myMainWindow
    timer = QTimer()
    timer.timeout.connect(ex.updateRun())
    timer.start(1000)
    sys.exit(app.exec_())
