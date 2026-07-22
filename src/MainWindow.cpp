#include "MainWindow.h"

#include "DetectionWorker.h"
#include "ImageUtils.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPixmap>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>

namespace
{
QString elidedPath(const QString &path)
{
    if (path.isEmpty()) {
        return QStringLiteral("-");
    }
    return QFileInfo(path).fileName();
}

QString csvEscape(const QString &value)
{
    QString escaped = value;
    escaped.replace('"', "\"\"");
    return QStringLiteral("\"%1\"").arg(escaped);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    setupWorkerThread();

    connect(&m_cameraTimer, &QTimer::timeout, this, &MainWindow::grabCameraFrame);
    appendLog(QStringLiteral("系统初始化完成，等待导入图片或启动摄像头。"));
}

MainWindow::~MainWindow()
{
    stopCamera();
    m_workerThread.quit();
    m_workerThread.wait();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("工业零件缺陷检测上位机 - Qt5/OpenCV"));
    resize(1280, 820);

    QWidget *central = new QWidget(this);
    QVBoxLayout *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(8);

    QPushButton *openImageButton = new QPushButton(QStringLiteral("导入图片"), this);
    QPushButton *openTemplateButton = new QPushButton(QStringLiteral("导入模板图"), this);
    QPushButton *clearTemplateButton = new QPushButton(QStringLiteral("清除模板"), this);
    QPushButton *openFolderButton = new QPushButton(QStringLiteral("导入文件夹"), this);
    m_prevButton = new QPushButton(QStringLiteral("上一张"), this);
    m_nextButton = new QPushButton(QStringLiteral("下一张"), this);
    m_startCameraButton = new QPushButton(QStringLiteral("启动摄像头"), this);
    m_stopCameraButton = new QPushButton(QStringLiteral("停止摄像头"), this);
    m_detectButton = new QPushButton(QStringLiteral("执行检测"), this);
    m_saveResultButton = new QPushButton(QStringLiteral("保存结果图"), this);

    m_stopCameraButton->setEnabled(false);
    m_saveResultButton->setEnabled(false);
    m_prevButton->setEnabled(false);
    m_nextButton->setEnabled(false);

    QHBoxLayout *toolbar = new QHBoxLayout;
    toolbar->addWidget(openImageButton);
    toolbar->addWidget(openTemplateButton);
    toolbar->addWidget(clearTemplateButton);
    toolbar->addSpacing(12);
    toolbar->addWidget(openFolderButton);
    toolbar->addWidget(m_prevButton);
    toolbar->addWidget(m_nextButton);
    toolbar->addSpacing(12);
    toolbar->addWidget(m_startCameraButton);
    toolbar->addWidget(m_stopCameraButton);
    toolbar->addSpacing(12);
    toolbar->addWidget(m_detectButton);
    toolbar->addWidget(m_saveResultButton);
    toolbar->addStretch();
    rootLayout->addLayout(toolbar);

    m_originalLabel = new QLabel(QStringLiteral("原图显示区"), this);
    m_resultLabel = new QLabel(QStringLiteral("检测结果显示区"), this);
    for (QLabel *label : {m_originalLabel, m_resultLabel}) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(420, 320);
        label->setStyleSheet(QStringLiteral("QLabel { background: #111827; color: #d1d5db; border: 1px solid #374151; }"));
    }

    QGridLayout *imageLayout = new QGridLayout;
    imageLayout->addWidget(new QLabel(QStringLiteral("原始图像"), this), 0, 0);
    imageLayout->addWidget(new QLabel(QStringLiteral("检测结果"), this), 0, 1);
    imageLayout->addWidget(m_originalLabel, 1, 0);
    imageLayout->addWidget(m_resultLabel, 1, 1);
    imageLayout->setColumnStretch(0, 1);
    imageLayout->setColumnStretch(1, 1);

    QGroupBox *paramBox = new QGroupBox(QStringLiteral("参数配置"), this);
    QFormLayout *paramLayout = new QFormLayout(paramBox);

    m_thresholdSpin = new QSpinBox(this);
    m_thresholdSpin->setRange(0, 255);
    m_thresholdSpin->setValue(40);

    m_blurSpin = new QSpinBox(this);
    m_blurSpin->setRange(1, 31);
    m_blurSpin->setSingleStep(2);
    m_blurSpin->setValue(3);

    m_morphSpin = new QSpinBox(this);
    m_morphSpin->setRange(1, 31);
    m_morphSpin->setSingleStep(2);
    m_morphSpin->setValue(3);

    m_minAreaSpin = new QDoubleSpinBox(this);
    m_minAreaSpin->setRange(1.0, 1000000.0);
    m_minAreaSpin->setValue(30.0);
    m_minAreaSpin->setDecimals(1);

    m_minWidthSpin = new QSpinBox(this);
    m_minWidthSpin->setRange(1, 10000);
    m_minWidthSpin->setValue(3);

    m_minHeightSpin = new QSpinBox(this);
    m_minHeightSpin->setRange(1, 10000);
    m_minHeightSpin->setValue(3);

    m_scratchAspectSpin = new QDoubleSpinBox(this);
    m_scratchAspectSpin->setRange(1.0, 100.0);
    m_scratchAspectSpin->setValue(4.0);
    m_scratchAspectSpin->setSingleStep(0.5);

    m_maxAspectSpin = new QDoubleSpinBox(this);
    m_maxAspectSpin->setRange(1.0, 1000.0);
    m_maxAspectSpin->setValue(25.0);
    m_maxAspectSpin->setSingleStep(1.0);

    m_invertCheck = new QCheckBox(QStringLiteral("反向阈值"), this);
    m_otsuCheck = new QCheckBox(QStringLiteral("Otsu 自动阈值"), this);

    paramLayout->addRow(QStringLiteral("阈值"), m_thresholdSpin);
    paramLayout->addRow(QStringLiteral("滤波核大小"), m_blurSpin);
    paramLayout->addRow(QStringLiteral("形态学核大小"), m_morphSpin);
    paramLayout->addRow(QStringLiteral("最小缺陷面积"), m_minAreaSpin);
    paramLayout->addRow(QStringLiteral("最小宽度"), m_minWidthSpin);
    paramLayout->addRow(QStringLiteral("最小高度"), m_minHeightSpin);
    paramLayout->addRow(QStringLiteral("划痕长宽比"), m_scratchAspectSpin);
    paramLayout->addRow(QStringLiteral("最大长宽比"), m_maxAspectSpin);
    paramLayout->addRow(m_invertCheck);
    paramLayout->addRow(m_otsuCheck);

    QGroupBox *statusBox = new QGroupBox(QStringLiteral("检测状态"), this);
    QFormLayout *statusLayout = new QFormLayout(statusBox);
    m_statusLabel = new QLabel(QStringLiteral("待检测"), this);
    m_imagePathLabel = new QLabel(QStringLiteral("-"), this);
    m_templatePathLabel = new QLabel(QStringLiteral("-"), this);
    m_countLabel = new QLabel(QStringLiteral("0"), this);
    m_timeLabel = new QLabel(QStringLiteral("0 ms"), this);
    statusLayout->addRow(QStringLiteral("结论"), m_statusLabel);
    statusLayout->addRow(QStringLiteral("当前图片"), m_imagePathLabel);
    statusLayout->addRow(QStringLiteral("模板图"), m_templatePathLabel);
    statusLayout->addRow(QStringLiteral("缺陷数量"), m_countLabel);
    statusLayout->addRow(QStringLiteral("处理耗时"), m_timeLabel);

    QVBoxLayout *sideLayout = new QVBoxLayout;
    sideLayout->addWidget(paramBox);
    sideLayout->addWidget(statusBox);
    sideLayout->addStretch();

    QHBoxLayout *middleLayout = new QHBoxLayout;
    middleLayout->addLayout(imageLayout, 1);
    middleLayout->addLayout(sideLayout);
    rootLayout->addLayout(middleLayout, 1);

    m_defectTable = new QTableWidget(this);
    m_defectTable->setColumnCount(5);
    m_defectTable->setHorizontalHeaderLabels(QStringList()
                                             << QStringLiteral("类型")
                                             << QStringLiteral("面积")
                                             << QStringLiteral("长宽比")
                                             << QStringLiteral("位置")
                                             << QStringLiteral("尺寸"));
    m_defectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_defectTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_defectTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(m_defectTable, 1);
    bottomLayout->addWidget(m_logEdit, 1);
    rootLayout->addLayout(bottomLayout);

    setCentralWidget(central);

    connect(openImageButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(openTemplateButton, &QPushButton::clicked, this, &MainWindow::openTemplateImage);
    connect(clearTemplateButton, &QPushButton::clicked, this, &MainWindow::clearTemplate);
    connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openImageFolder);
    connect(m_prevButton, &QPushButton::clicked, this, &MainWindow::showPreviousImage);
    connect(m_nextButton, &QPushButton::clicked, this, &MainWindow::showNextImage);
    connect(m_startCameraButton, &QPushButton::clicked, this, &MainWindow::startCamera);
    connect(m_stopCameraButton, &QPushButton::clicked, this, &MainWindow::stopCamera);
    connect(m_detectButton, &QPushButton::clicked, this, &MainWindow::detectCurrentImage);
    connect(m_saveResultButton, &QPushButton::clicked, this, &MainWindow::saveCurrentResult);
}

void MainWindow::setupWorkerThread()
{
    m_worker = new DetectionWorker;
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &DetectionWorker::detectionFinished, this, &MainWindow::onDetectionFinished);
    m_workerThread.start();
}

void MainWindow::openImage()
{
    stopCamera();
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          QStringLiteral("选择待检测图片"),
                                                          QDir::currentPath(),
                                                          supportedImageNameFilters().join(QStringLiteral(";;")),
                                                          nullptr,
                                                          QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) {
        return;
    }

    m_folderImages.clear();
    m_folderIndex = -1;
    m_prevButton->setEnabled(false);
    m_nextButton->setEnabled(false);
    loadImageToPreview(filePath);
    requestFileDetection(filePath);
}

void MainWindow::openTemplateImage()
{
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          QStringLiteral("选择 DeepPCB 模板图"),
                                                          QDir::currentPath(),
                                                          supportedImageNameFilters().join(QStringLiteral(";;")),
                                                          nullptr,
                                                          QFileDialog::DontUseNativeDialog);
    if (filePath.isEmpty()) {
        return;
    }
    
    m_templatePath = filePath;
    m_templatePathLabel->setText(elidedPath(filePath));
    appendLog(QStringLiteral("已导入模板图：%1").arg(filePath));
    if (!m_currentImagePath.isEmpty()) {
        requestFileDetection(m_currentImagePath);
    }
}

void MainWindow::openImageFolder()
{
    stopCamera();
    const QString folderPath = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择图片文件夹"),
        QDir::currentPath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    if (folderPath.isEmpty()) {
        return;
    }

    const QStringList extensions = QStringList() << QStringLiteral("*.jpg") << QStringLiteral("*.jpeg")
                                                 << QStringLiteral("*.png") << QStringLiteral("*.bmp")
                                                 << QStringLiteral("*.tif") << QStringLiteral("*.tiff");
    QDirIterator iterator(folderPath, extensions, QDir::Files, QDirIterator::Subdirectories);
    m_folderImages.clear();
    while (iterator.hasNext()) {
        m_folderImages << iterator.next();
    }
    m_folderImages.sort();

    if (m_folderImages.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("该文件夹下没有找到支持的图片文件。"));
        return;
    }

    m_folderIndex = 0;
    m_prevButton->setEnabled(m_folderImages.size() > 1);
    m_nextButton->setEnabled(m_folderImages.size() > 1);
    appendLog(QStringLiteral("已导入文件夹：%1，共 %2 张图片。").arg(folderPath).arg(m_folderImages.size()));
    loadImageToPreview(m_folderImages[m_folderIndex]);
    requestFileDetection(m_folderImages[m_folderIndex]);
}

void MainWindow::showPreviousImage()
{
    if (m_folderImages.isEmpty()) {
        return;
    }
    m_folderIndex = (m_folderIndex - 1 + m_folderImages.size()) % m_folderImages.size();
    loadImageToPreview(m_folderImages[m_folderIndex]);
    requestFileDetection(m_folderImages[m_folderIndex]);
}

void MainWindow::showNextImage()
{
    if (m_folderImages.isEmpty()) {
        return;
    }
    m_folderIndex = (m_folderIndex + 1) % m_folderImages.size();
    loadImageToPreview(m_folderImages[m_folderIndex]);
    requestFileDetection(m_folderImages[m_folderIndex]);
}

void MainWindow::detectCurrentImage()
{
    if (!m_currentImagePath.isEmpty()) {
        requestFileDetection(m_currentImagePath);
        return;
    }

    if (!m_currentOriginal.isNull()) {
        if (m_detectionBusy) {
            return;
        }
        setBusy(true);
        DetectorParams params = currentParams();
        const bool submitted = QMetaObject::invokeMethod(m_worker,
                                                         "detectImage",
                                                         Qt::QueuedConnection,
                                                         Q_ARG(QImage, m_currentOriginal),
                                                         Q_ARG(QString, QStringLiteral("camera-frame")),
                                                         Q_ARG(DetectorParams, params));
        if (!submitted) {
            setBusy(false);
            appendLog(QStringLiteral("当前摄像头帧检测任务提交失败。"));
            return;
        }
        appendLog(QStringLiteral("已提交当前摄像头帧检测任务。"));
        return;
    }

    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先导入图片或启动摄像头。"));
}

void MainWindow::startCamera()
{
    if (m_camera.isOpened()) {
        return;
    }

    if (!m_camera.open(0)) {
        QMessageBox::warning(this, QStringLiteral("摄像头错误"), QStringLiteral("无法打开默认摄像头。"));
        appendLog(QStringLiteral("摄像头打开失败。"));
        return;
    }

    m_currentImagePath.clear();
    m_imagePathLabel->setText(QStringLiteral("camera:0"));
    m_startCameraButton->setEnabled(false);
    m_stopCameraButton->setEnabled(true);
    m_cameraTimer.start(120);
    appendLog(QStringLiteral("默认摄像头已启动。"));
}

void MainWindow::stopCamera()
{
    if (m_cameraTimer.isActive()) {
        m_cameraTimer.stop();
    }
    if (m_camera.isOpened()) {
        m_camera.release();
        appendLog(QStringLiteral("摄像头已停止。"));
    }
    if (m_startCameraButton) {
        m_startCameraButton->setEnabled(true);
    }
    if (m_stopCameraButton) {
        m_stopCameraButton->setEnabled(false);
    }
}

void MainWindow::grabCameraFrame()
{
    if (!m_camera.isOpened()) {
        return;
    }

    cv::Mat frame;
    m_camera >> frame;
    if (frame.empty()) {
        appendLog(QStringLiteral("摄像头帧为空。"));
        return;
    }

    m_currentOriginal = ImageUtils::cvMatToQImage(frame);
    displayImage(m_originalLabel, m_currentOriginal);

    if (!m_detectionBusy) {
        setBusy(true);
        DetectorParams params = currentParams();
        const bool submitted = QMetaObject::invokeMethod(m_worker,
                                                         "detectImage",
                                                         Qt::QueuedConnection,
                                                         Q_ARG(QImage, m_currentOriginal),
                                                         Q_ARG(QString, QStringLiteral("camera-frame")),
                                                         Q_ARG(DetectorParams, params));
        if (!submitted) {
            setBusy(false);
            appendLog(QStringLiteral("摄像头帧检测任务提交失败。"));
        }
    }
}

void MainWindow::saveCurrentResult()
{
    if (m_currentResult.isNull()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前没有可保存的检测结果图。"));
        return;
    }

    QDir resultDir(QCoreApplication::applicationDirPath() + QStringLiteral("/results"));
    if (!resultDir.exists()) {
        resultDir.mkpath(QStringLiteral("."));
    }

    const QString defaultName = resultDir.filePath(QStringLiteral("result_%1.png")
                                                   .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"))));
    const QString savePath = QFileDialog::getSaveFileName(this,
                                                          QStringLiteral("保存检测结果图"),
                                                          defaultName,
                                                          QStringLiteral("PNG Image (*.png);;JPEG Image (*.jpg)"),
                                                          nullptr,
                                                          QFileDialog::DontUseNativeDialog);
    if (savePath.isEmpty()) {
        return;
    }

    if (m_currentResult.save(savePath)) {
        appendLog(QStringLiteral("结果图已保存：%1").arg(savePath));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("结果图保存失败。"));
    }
}

void MainWindow::clearTemplate()
{
    m_templatePath.clear();
    m_templatePathLabel->setText(QStringLiteral("-"));
    appendLog(QStringLiteral("已清除模板图，后续检测使用普通阈值分割模式。"));
}

void MainWindow::onDetectionFinished(const DetectionResult &result)
{
    setBusy(false);

    if (!result.success) {
        m_statusLabel->setText(QStringLiteral("错误"));
        appendLog(result.errorMessage);
        return;
    }

    m_currentOriginal = result.originalImage;
    m_currentResult = result.resultImage;
    displayImage(m_originalLabel, result.originalImage);
    displayImage(m_resultLabel, result.resultImage);
    m_saveResultButton->setEnabled(true);

    m_statusLabel->setText(result.conclusion);
    if (result.conclusion == QStringLiteral("NG")) {
        m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #dc2626; font-weight: bold; }"));
    } else {
        m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #16a34a; font-weight: bold; }"));
    }
    m_countLabel->setText(QString::number(result.defectCount));
    m_timeLabel->setText(QStringLiteral("%1 ms").arg(result.elapsedMs, 0, 'f', 0));

    updateDefectTable(result);
    appendCsvRecord(result);
    appendLog(QStringLiteral("检测完成：%1，结论=%2，缺陷数量=%3，耗时=%4 ms")
              .arg(result.sourcePath)
              .arg(result.conclusion)
              .arg(result.defectCount)
              .arg(result.elapsedMs, 0, 'f', 0));
}

void MainWindow::appendLog(const QString &message)
{
    if (!m_logEdit) {
        return;
    }
    const QString line = QStringLiteral("[%1] %2")
                         .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")))
                         .arg(message);
    m_logEdit->appendPlainText(line);
}

void MainWindow::displayImage(QLabel *label, const QImage &image)
{
    if (!label || image.isNull()) {
        return;
    }

    label->setPixmap(QPixmap::fromImage(image).scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::loadImageToPreview(const QString &imagePath)
{
    QImage image(imagePath);
    if (image.isNull()) {
        QMessageBox::warning(this, QStringLiteral("读取失败"), QStringLiteral("无法读取图片：%1").arg(imagePath));
        return;
    }

    m_currentImagePath = imagePath;
    m_currentOriginal = image;
    m_currentResult = QImage();
    m_imagePathLabel->setText(elidedPath(imagePath));
    m_statusLabel->setStyleSheet(QString());
    m_statusLabel->setText(QStringLiteral("待检测"));
    m_countLabel->setText(QStringLiteral("0"));
    m_timeLabel->setText(QStringLiteral("0 ms"));
    m_defectTable->setRowCount(0);
    displayImage(m_originalLabel, image);
    m_resultLabel->setPixmap(QPixmap());
    m_resultLabel->setText(QStringLiteral("检测结果显示区"));
    m_saveResultButton->setEnabled(false);
    appendLog(QStringLiteral("已加载图片：%1").arg(imagePath));
}

void MainWindow::requestFileDetection(const QString &imagePath)
{
    if (imagePath.isEmpty() || m_detectionBusy) {
        return;
    }

    setBusy(true);
    DetectorParams params = currentParams();
    const bool submitted = QMetaObject::invokeMethod(m_worker,
                                                     "detectFile",
                                                     Qt::QueuedConnection,
                                                     Q_ARG(QString, imagePath),
                                                     Q_ARG(DetectorParams, params));
    if (!submitted) {
        setBusy(false);
        appendLog(QStringLiteral("图片检测任务提交失败：%1").arg(imagePath));
        return;
    }
    appendLog(QStringLiteral("已提交图片检测任务：%1").arg(imagePath));
}

DetectorParams MainWindow::currentParams() const
{
    DetectorParams params;
    params.thresholdValue = m_thresholdSpin->value();
    params.blurKernel = m_blurSpin->value();
    params.morphKernel = m_morphSpin->value();
    params.minArea = m_minAreaSpin->value();
    params.minWidth = m_minWidthSpin->value();
    params.minHeight = m_minHeightSpin->value();
    params.scratchAspectRatio = m_scratchAspectSpin->value();
    params.maxAspectRatio = m_maxAspectSpin->value();
    params.invertThreshold = m_invertCheck->isChecked();
    params.useOtsu = m_otsuCheck->isChecked();
    params.templatePath = m_templatePath;
    return params;
}

QStringList MainWindow::supportedImageNameFilters() const
{
    return QStringList()
           << QStringLiteral("Images (*.jpg *.jpeg *.png *.bmp *.tif *.tiff)")
           << QStringLiteral("All Files (*)");
}

void MainWindow::appendCsvRecord(const DetectionResult &result)
{
    QDir logDir(QCoreApplication::applicationDirPath() + QStringLiteral("/logs"));
    if (!logDir.exists()) {
        logDir.mkpath(QStringLiteral("."));
    }

    QFile file(logDir.filePath(QStringLiteral("detection_records.csv")));
    const bool needHeader = !file.exists() || file.size() == 0;
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        appendLog(QStringLiteral("CSV 记录写入失败：%1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    if (needHeader) {
        stream << "time,image_path,template_path,defect_count,conclusion,elapsed_ms\n";
    }
    stream << csvEscape(result.detectionTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))) << ','
           << csvEscape(result.sourcePath) << ','
           << csvEscape(m_templatePath) << ','
           << result.defectCount << ','
           << csvEscape(result.conclusion) << ','
           << QString::number(result.elapsedMs, 'f', 0) << '\n';
}

void MainWindow::updateDefectTable(const DetectionResult &result)
{
    m_defectTable->setRowCount(result.defects.size());
    for (int row = 0; row < result.defects.size(); ++row) {
        const DefectInfo &defect = result.defects[row];
        const QRect rect = defect.boundingBox;

        m_defectTable->setItem(row, 0, new QTableWidgetItem(defect.defectType));
        m_defectTable->setItem(row, 1, new QTableWidgetItem(QString::number(defect.area, 'f', 1)));
        m_defectTable->setItem(row, 2, new QTableWidgetItem(QString::number(defect.aspectRatio, 'f', 2)));
        m_defectTable->setItem(row, 3, new QTableWidgetItem(QStringLiteral("(%1,%2)").arg(rect.x()).arg(rect.y())));
        m_defectTable->setItem(row, 4, new QTableWidgetItem(QStringLiteral("%1x%2").arg(rect.width()).arg(rect.height())));
    }
}

void MainWindow::setBusy(bool busy)
{
    m_detectionBusy = busy;
    m_detectButton->setEnabled(!busy);
    if (busy) {
        m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: #2563eb; font-weight: bold; }"));
        m_statusLabel->setText(QStringLiteral("检测中"));
    }
}
