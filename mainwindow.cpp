#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QDialog>
#include <QSettings>
#include <QFileDialog>
#include <QCheckBox>
#include <QHBoxLayout>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace {
    QWidget* createCenteredCheckBox(bool checked) {
        QWidget *container = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignCenter);
        QCheckBox *checkBox = new QCheckBox(container);
        checkBox->setChecked(checked);
        layout->addWidget(checkBox);
        return container;
    }
}

// Класс для захвата нажатия клавиши
class KeyCaptureDialog : public QDialog {
public:
    int capturedKey;
    KeyCaptureDialog(QWidget *parent = nullptr) : QDialog(parent), capturedKey(0) {
        setWindowTitle("Нажмите клавишу");
        setModal(true);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }
protected:
    void keyPressEvent(QKeyEvent *event) override {
        capturedKey = event->key();
        accept();
    }
};

// Класс для выбора точки на скриншоте
class ScreenPicker : public QLabel {
    Q_OBJECT
public:
    ScreenPicker(const QPixmap &pixmap, QWidget *parent = nullptr) : QLabel(parent) {
         setPixmap(pixmap);
         setScaledContents(true);
         setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
         setWindowState(Qt::WindowFullScreen);
         setCursor(Qt::CrossCursor);
    }
signals:
    void pointPicked(const QPoint &pt);
protected:
    void mousePressEvent(QMouseEvent *event) override {
         emit pointPicked(event->pos());
         close();
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , trackingActive(false)
{
    ui->setupUi(this);

    // Инициализация таймера для проверки условий
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::checkConditions);

    // Подключение сигналов от кнопок
    connect(ui->toggleButton, &QPushButton::clicked, this, &MainWindow::onToggleTracking);
    connect(ui->saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfigClicked);
    connect(ui->loadConfigButton, &QPushButton::clicked, this, &MainWindow::onLoadConfigClicked);
    connect(ui->addPointButton, &QPushButton::clicked, this, &MainWindow::onAddPointClicked);
    connect(ui->removePointButton, &QPushButton::clicked, this, &MainWindow::onRemovePointClicked);
    connect(ui->pickTrackingButton, &QPushButton::clicked, this, &MainWindow::onPickTrackingPointClicked);
    connect(ui->scanKeyButton, &QPushButton::clicked, this, &MainWindow::onScanKeyClicked);

    ui->statusLabel->setText("Опрос: не идет");

    // Настройка таблицы точек отслеживания (4 колонки: X, Y, Color, Не равен)
    ui->trackingTable->setColumnCount(4);
    QStringList headers;
    headers << "X" << "Y" << "Color" << "Не равен";
    ui->trackingTable->setHorizontalHeaderLabels(headers);

    // Загрузка последнего профиля из реестра (если имеется)
    QSettings settings("MyCompany", "PoeLifer");
    QString lastProfile = settings.value("lastProfilePath", "").toString();
    if (!lastProfile.isEmpty()) {
        loadProfileFromFile(lastProfile);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onToggleTracking()
{
    trackingActive = !trackingActive;
    if (trackingActive) {
        ui->toggleButton->setText("Стоп");
        ui->statusLabel->setText("Опрос: идет");
        timer->start(50);
    } else {
        ui->toggleButton->setText("Старт");
        ui->statusLabel->setText("Опрос: не идет");
        timer->stop();
    }
}

void MainWindow::checkConditions()
{
    if (!trackingActive)
        return;

    int rowCount = ui->trackingTable->rowCount();
    if (rowCount == 0)
        return;

    // Проходим по всем точкам. Последняя точка используется как финальная.
    for (int i = 0; i < rowCount; i++) {
        QTableWidgetItem *xItem = ui->trackingTable->item(i, 0);
        QTableWidgetItem *yItem = ui->trackingTable->item(i, 1);
        QTableWidgetItem *colorItem = ui->trackingTable->item(i, 2);
        if (!xItem || !yItem || !colorItem)
            continue;
        bool okX, okY;
        int x = xItem->text().toInt(&okX);
        int y = yItem->text().toInt(&okY);
        if (!okX || !okY)
            continue;
        QColor expectedColor(colorItem->text().trimmed());
        QColor currentColor = getPixelColor(x, y);

        QWidget *widget = ui->trackingTable->cellWidget(i, 3);
        QCheckBox *checkBox = widget ? widget->findChild<QCheckBox*>() : nullptr;
        bool invert = (checkBox && checkBox->isChecked());

        if (!invert) {
            if (currentColor != expectedColor)
                return;
        } else {
            if (currentColor == expectedColor)
                return;
        }
    }

#ifdef Q_OS_WIN
    // Если все точки удовлетворяют условию, симулируем нажатие клавиши с задержкой
    bool ok;
    int keyCode = ui->keyLineEdit->text().toInt(&ok, 0);
    if (!ok)
        return;

    INPUT inputDown = {};
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = static_cast<WORD>(keyCode);
    inputDown.ki.dwFlags = 0;
    SendInput(1, &inputDown, sizeof(INPUT));

    int pressDelay = QRandomGenerator::global()->bounded(50, 151);
    QTimer::singleShot(pressDelay, [this, keyCode]() {
         INPUT inputUp = {};
         inputUp.type = INPUT_KEYBOARD;
         inputUp.ki.wVk = static_cast<WORD>(keyCode);
         inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
         SendInput(1, &inputUp, sizeof(INPUT));
         Beep(750, 100);
    });
#endif

    // После выполнения, делаем паузу перед следующим циклом
    timer->stop();
    int delay = QRandomGenerator::global()->bounded(200, 401);
    QTimer::singleShot(delay, [this]() {
        if (trackingActive) {
            timer->start(50);
        }
    });
}

QString MainWindow::getActiveProcessName()
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return "";
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        TCHAR processName[MAX_PATH] = TEXT("<unknown>");
        if (GetModuleBaseName(hProcess, NULL, processName, sizeof(processName)/sizeof(TCHAR))) {
            CloseHandle(hProcess);
            return QString::fromWCharArray(processName);
        }
        CloseHandle(hProcess);
    }
#endif
    return "";
}

QColor MainWindow::getPixelColor(int x, int y)
{
#ifdef Q_OS_WIN
    HDC hdcScreen = GetDC(nullptr);
    COLORREF color = GetPixel(hdcScreen, x, y);
    ReleaseDC(nullptr, hdcScreen);
    return QColor(GetRValue(color), GetGValue(color), GetBValue(color));
#else
    return QColor();
#endif
}

void MainWindow::onAddPointClicked()
{
    int row = ui->trackingTable->rowCount();
    ui->trackingTable->insertRow(row);
    ui->trackingTable->setItem(row, 0, new QTableWidgetItem("0"));
    ui->trackingTable->setItem(row, 1, new QTableWidgetItem("0"));
    ui->trackingTable->setItem(row, 2, new QTableWidgetItem("#FFFFFF"));
    ui->trackingTable->setCellWidget(row, 3, createCenteredCheckBox(false));
}

void MainWindow::onRemovePointClicked()
{
    QList<QTableWidgetItem *> selected = ui->trackingTable->selectedItems();
    if (selected.isEmpty())
        return;
    int row = selected.first()->row();
    ui->trackingTable->removeRow(row);
}

void MainWindow::onSaveConfigClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Сохранить профиль", "", "JSON Files (*.json)");
    if (filePath.isEmpty())
        return;
    saveProfileToFile(filePath);
    QMessageBox::information(this, "Конфигурация", "Конфигурация сохранена");
}

void MainWindow::onLoadConfigClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Загрузить профиль", "", "JSON Files (*.json)");
    if (filePath.isEmpty())
        return;
    loadProfileFromFile(filePath);
    QMessageBox::information(this, "Конфигурация", "Конфигурация загружена");
}

void MainWindow::loadProfileFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;
    QJsonObject obj = doc.object();

    ui->processLineEdit->setText(obj.value("processName").toString());
    ui->keyLineEdit->setText(obj.value("key").toString());

    ui->trackingTable->setRowCount(0);
    QJsonArray points = obj.value("trackingPoints").toArray();
    for (int i = 0; i < points.size(); i++) {
        QJsonObject point = points[i].toObject();
        int x = point.value("x").toInt();
        int y = point.value("y").toInt();
        QString color = point.value("color").toString();
        bool invert = point.value("invert").toBool();
        int row = ui->trackingTable->rowCount();
        ui->trackingTable->insertRow(row);
        ui->trackingTable->setItem(row, 0, new QTableWidgetItem(QString::number(x)));
        ui->trackingTable->setItem(row, 1, new QTableWidgetItem(QString::number(y)));
        ui->trackingTable->setItem(row, 2, new QTableWidgetItem(color));
        ui->trackingTable->setCellWidget(row, 3, createCenteredCheckBox(invert));
    }

    QSettings settings("MyCompany", "PoeLifer");
    settings.setValue("lastProfilePath", filePath);
}

void MainWindow::saveProfileToFile(const QString &filePath)
{
    QJsonObject obj;
    obj["processName"] = ui->processLineEdit->text();
    obj["key"] = ui->keyLineEdit->text();

    QJsonArray points;
    int rows = ui->trackingTable->rowCount();
    for (int i = 0; i < rows; i++) {
        QTableWidgetItem *xItem = ui->trackingTable->item(i, 0);
        QTableWidgetItem *yItem = ui->trackingTable->item(i, 1);
        QTableWidgetItem *colorItem = ui->trackingTable->item(i, 2);
        if (!xItem || !yItem || !colorItem)
            continue;
        bool okX, okY;
        int x = xItem->text().toInt(&okX);
        int y = yItem->text().toInt(&okY);
        if (!okX || !okY)
            continue;
        QString color = colorItem->text();

        QWidget *widget = ui->trackingTable->cellWidget(i, 3);
        QCheckBox *checkBox = widget ? widget->findChild<QCheckBox*>() : nullptr;
        bool invert = (checkBox && checkBox->isChecked());

        QJsonObject point;
        point["x"] = x;
        point["y"] = y;
        point["color"] = color;
        point["invert"] = invert;
        points.append(point);
    }
    obj["trackingPoints"] = points;
    obj["timeoutMin"] = 200;
    obj["timeoutMax"] = 400;

    QJsonDocument doc(obj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
    QSettings settings("MyCompany", "PoeLifer");
    settings.setValue("lastProfilePath", filePath);
}

void MainWindow::onPickTrackingPointClicked()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QPixmap screenshot = screen->grabWindow(0);
    ScreenPicker *picker = new ScreenPicker(screenshot);
    connect(picker, &ScreenPicker::pointPicked, this, [this, screenshot](const QPoint &pt) {
         QColor color = screenshot.toImage().pixelColor(pt);
         int row = ui->trackingTable->rowCount();
         ui->trackingTable->insertRow(row);
         ui->trackingTable->setItem(row, 0, new QTableWidgetItem(QString::number(pt.x())));
         ui->trackingTable->setItem(row, 1, new QTableWidgetItem(QString::number(pt.y())));
         ui->trackingTable->setItem(row, 2, new QTableWidgetItem(color.name()));
         ui->trackingTable->setCellWidget(row, 3, createCenteredCheckBox(false));
    });
    picker->show();
}

void MainWindow::onScanKeyClicked()
{
    KeyCaptureDialog dialog(this);
    dialog.exec();
    int key = dialog.capturedKey;
    ui->keyLineEdit->setText(QString::number(key));
}

#include "mainwindow.moc"