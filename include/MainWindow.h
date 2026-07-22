#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DetectionTypes.h"

#include <QDateTime>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStringList>
#include <QThread>
#include <QTimer>

#include "OpenCvCompat.h"

class QCheckBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTableWidget;

class DetectionWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void openImage();
    void openTemplateImage();
    void openImageFolder();
    void showPreviousImage();
    void showNextImage();
    void detectCurrentImage();
    void startCamera();
    void stopCamera();
    void grabCameraFrame();
    void saveCurrentResult();
    void clearTemplate();
    void onDetectionFinished(const DetectionResult &result);

private:
    void buildUi();
    void setupWorkerThread();
    void appendLog(const QString &message);
    void displayImage(QLabel *label, const QImage &image);
    void loadImageToPreview(const QString &imagePath);
    void requestFileDetection(const QString &imagePath);
    DetectorParams currentParams() const;
    QStringList supportedImageNameFilters() const;
    void appendCsvRecord(const DetectionResult &result);
    void updateDefectTable(const DetectionResult &result);
    void setBusy(bool busy);

    QLabel *m_originalLabel = nullptr;
    QLabel *m_resultLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_imagePathLabel = nullptr;
    QLabel *m_templatePathLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QTableWidget *m_defectTable = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;

    QSpinBox *m_thresholdSpin = nullptr;
    QSpinBox *m_blurSpin = nullptr;
    QSpinBox *m_morphSpin = nullptr;
    QDoubleSpinBox *m_minAreaSpin = nullptr;
    QSpinBox *m_minWidthSpin = nullptr;
    QSpinBox *m_minHeightSpin = nullptr;
    QDoubleSpinBox *m_scratchAspectSpin = nullptr;
    QDoubleSpinBox *m_maxAspectSpin = nullptr;
    QCheckBox *m_invertCheck = nullptr;
    QCheckBox *m_otsuCheck = nullptr;

    QPushButton *m_detectButton = nullptr;
    QPushButton *m_startCameraButton = nullptr;
    QPushButton *m_stopCameraButton = nullptr;
    QPushButton *m_saveResultButton = nullptr;
    QPushButton *m_prevButton = nullptr;
    QPushButton *m_nextButton = nullptr;

    DetectionWorker *m_worker = nullptr;
    QThread m_workerThread;
    bool m_detectionBusy = false;

    QString m_currentImagePath;
    QString m_templatePath;
    QImage m_currentOriginal;
    QImage m_currentResult;
    QStringList m_folderImages;
    int m_folderIndex = -1;

    QTimer m_cameraTimer;
    cv::VideoCapture m_camera;
};

#endif
