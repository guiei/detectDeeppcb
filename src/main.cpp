#include "MainWindow.h"
#include "DetectionTypes.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<DetectorParams>("DetectorParams");
    qRegisterMetaType<DefectInfo>("DefectInfo");
    qRegisterMetaType<DetectionResult>("DetectionResult");

    MainWindow window;
    window.show();

    return app.exec();
}
