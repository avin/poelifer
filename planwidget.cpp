#include "planwidget.h"
#include "networkutil.h"
#include "keycapturedialog.h"
#include "screenpicker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QCheckBox>
#include <QTimer>
#include <QDialog>
#include <QRandomGenerator>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QJsonArray>
#include <QGuiApplication>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

PlanWidget::PlanWidget(QWidget *parent) : QWidget(parent), planActive(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Инпут для имени плана
    planNameLineEdit = new QLineEdit(this);
    planNameLineEdit->setPlaceholderText("Имя плана");
    planNameLineEdit->setObjectName("planNameLineEdit");
    mainLayout->addWidget(planNameLineEdit);
    connect(planNameLineEdit, &QLineEdit::textChanged, this, [this](const QString &name){
        emit planNameChanged(name);
    });

    // Группа точек отслеживания
    QGroupBox *groupTracking = new QGroupBox("Точки отслеживания", this);
    QVBoxLayout *trackingLayout = new QVBoxLayout(groupTracking);
    QTableWidget *table = new QTableWidget(0, 4, this);
    table->setHorizontalHeaderLabels(QStringList() << "X" << "Y" << "Color" << "Не равен");
    table->horizontalHeader()->setStretchLastSection(true);
    table->setObjectName("trackingTable");
    trackingLayout->addWidget(table);
    QHBoxLayout *trackingButtons = new QHBoxLayout();
    QPushButton *addPointButton = new QPushButton("Добавить", this);
    QPushButton *removePointButton = new QPushButton("Удалить", this);
    QPushButton *pickPointButton = new QPushButton("Выбрать с экрана", this);
    trackingButtons->addWidget(addPointButton);
    trackingButtons->addWidget(removePointButton);
    trackingButtons->addWidget(pickPointButton);
    trackingLayout->addLayout(trackingButtons);
    mainLayout->addWidget(groupTracking);

    // Группа действия
    QGroupBox *groupAction = new QGroupBox("Действие", this);
    QHBoxLayout *actionLayout = new QHBoxLayout(groupAction);
    QComboBox *actionComboBox = new QComboBox(this);
    actionComboBox->addItem("Нажатие клавиши");
    actionComboBox->addItem("Дисконнект сети");
    actionComboBox->setObjectName("actionComboBox");
    actionLayout->addWidget(actionComboBox);
    QLineEdit *keyLineEdit = new QLineEdit(this);
    keyLineEdit->setPlaceholderText("Код клавиши");
    keyLineEdit->setObjectName("keyLineEdit");
    actionLayout->addWidget(keyLineEdit);
    QPushButton *scanKeyButton = new QPushButton("Сканировать", this);
    actionLayout->addWidget(scanKeyButton);
    mainLayout->addWidget(groupAction);

    // Кнопка старта/стопа плана
    QPushButton *toggleButton = new QPushButton("Старт", this);
    toggleButton->setObjectName("toggleButton");
    mainLayout->addWidget(toggleButton);

    setLayout(mainLayout);

    timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer, &QTimer::timeout, this, &PlanWidget::checkConditions);
    connect(toggleButton, &QPushButton::clicked, this, &PlanWidget::onTogglePlan);
    connect(addPointButton, &QPushButton::clicked, this, &PlanWidget::onAddPoint);
    connect(removePointButton, &QPushButton::clicked, this, &PlanWidget::onRemovePoint);
    connect(pickPointButton, &QPushButton::clicked, this, &PlanWidget::onPickPoint);
    connect(scanKeyButton, &QPushButton::clicked, this, &PlanWidget::onScanKey);
}

PlanWidget::~PlanWidget()
{
}

void PlanWidget::setProcessName(const QString &name)
{
    processName = name;
}

QString PlanWidget::getProcessName() const
{
    return processName;
}

void PlanWidget::setPlanName(const QString &name)
{
    if(planNameLineEdit)
        planNameLineEdit->setText(name);
}

QString PlanWidget::getPlanName() const
{
    return planNameLineEdit ? planNameLineEdit->text() : QString();
}

bool PlanWidget::isPlanActive() const
{
    return planActive;
}

void PlanWidget::onTogglePlan()
{
    QPushButton *toggleButton = findChild<QPushButton*>("toggleButton");
    if (!planActive) {
        planActive = true;
        if (toggleButton)
            toggleButton->setText("Стоп");
        timer->start();
    } else {
        planActive = false;
        if (toggleButton)
            toggleButton->setText("Старт");
        timer->stop();
    }
    emit statusChanged(planActive);
}

void PlanWidget::startPlan()
{
    if (!planActive) {
        onTogglePlan();
    }
}

void PlanWidget::stopPlan()
{
    if (planActive) {
        onTogglePlan();
    }
}

void PlanWidget::onAddPoint()
{
    QTableWidget *table = findChild<QTableWidget*>("trackingTable");
    if (!table)
        return;
    int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem("0"));
    table->setItem(row, 1, new QTableWidgetItem("0"));
    table->setItem(row, 2, new QTableWidgetItem("#FFFFFF"));
    QWidget *container = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0,0,0,0);
    layout->setAlignment(Qt::AlignCenter);
    QCheckBox *checkBox = new QCheckBox(container);
    layout->addWidget(checkBox);
    table->setCellWidget(row, 3, container);
}

void PlanWidget::onRemovePoint()
{
    QTableWidget *table = findChild<QTableWidget*>("trackingTable");
    if (!table)
        return;
    QList<QTableWidgetItem*> selected = table->selectedItems();
    if (selected.isEmpty())
        return;
    int row = selected.first()->row();
    table->removeRow(row);
}

void PlanWidget::onPickPoint()
{
    QTableWidget *table = findChild<QTableWidget*>("trackingTable");
    if (!table)
        return;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    QPixmap screenshot = screen->grabWindow(0);
    ScreenPicker *picker = new ScreenPicker(screenshot);
    connect(picker, &ScreenPicker::pointPicked, this, [this, table, screenshot](const QPoint &pt) {
         QColor color = screenshot.toImage().pixelColor(pt);
         int row = table->rowCount();
         table->insertRow(row);
         table->setItem(row, 0, new QTableWidgetItem(QString::number(pt.x())));
         table->setItem(row, 1, new QTableWidgetItem(QString::number(pt.y())));
         table->setItem(row, 2, new QTableWidgetItem(color.name()));
         QWidget *container = new QWidget(this);
         QHBoxLayout *layout = new QHBoxLayout(container);
         layout->setContentsMargins(0,0,0,0);
         layout->setAlignment(Qt::AlignCenter);
         QCheckBox *checkBox = new QCheckBox(container);
         layout->addWidget(checkBox);
         table->setCellWidget(row, 3, container);
    });
    picker->show();
}

void PlanWidget::onScanKey()
{
    KeyCaptureDialog dialog(this);
    if(dialog.exec() == QDialog::Accepted) {
         QLineEdit *keyLineEdit = findChild<QLineEdit*>("keyLineEdit");
         if (keyLineEdit) {
             keyLineEdit->setText(QString::number(dialog.capturedKey()));
         }
    }
}

bool PlanWidget::arePixelConditionsMet()
{
    QTableWidget *table = findChild<QTableWidget*>("trackingTable");
    if (!table)
        return false;
    int rowCount = table->rowCount();
    if (rowCount == 0)
        return false;
    for (int i = 0; i < rowCount; i++) {
        QTableWidgetItem *xItem = table->item(i, 0);
        QTableWidgetItem *yItem = table->item(i, 1);
        QTableWidgetItem *colorItem = table->item(i, 2);
        if (!xItem || !yItem || !colorItem)
            continue;
        bool okX, okY;
        int x = xItem->text().toInt(&okX);
        int y = yItem->text().toInt(&okY);
        if (!okX || !okY)
            continue;
        QColor expectedColor(colorItem->text().trimmed());
        QColor currentColor;
#ifdef Q_OS_WIN
        HDC hdcScreen = GetDC(nullptr);
        COLORREF col = GetPixel(hdcScreen, x, y);
        ReleaseDC(nullptr, hdcScreen);
        currentColor = QColor(GetRValue(col), GetGValue(col), GetBValue(col));
#else
        currentColor = QColor();
#endif
        QWidget *widget = table->cellWidget(i, 3);
        QCheckBox *checkBox = widget ? widget->findChild<QCheckBox*>() : nullptr;
        bool invert = (checkBox && checkBox->isChecked());
        if (!invert) {
            if (currentColor != expectedColor)
                return false;
        } else {
            if (currentColor == expectedColor)
                return false;
        }
    }
    return true;
}

void PlanWidget::checkConditions()
{
    if (!planActive)
        return;
    if (!arePixelConditionsMet())
        return;
    if (!arePixelConditionsMet())
        return;
    QComboBox *actionComboBox = findChild<QComboBox*>("actionComboBox");
    QLineEdit *keyLineEdit = findChild<QLineEdit*>("keyLineEdit");
    if (!actionComboBox || !keyLineEdit)
        return;
    QString action = actionComboBox->currentText();
#ifdef Q_OS_WIN
    if (action == "Дисконнект сети") {
        performDisconnect();
    } else {
        bool ok;
        int keyCode = keyLineEdit->text().toInt(&ok, 0);
        if (!ok)
            return;
        performKeyAction(keyCode);
    }
#endif
    timer->stop();
    int delay = QRandomGenerator::global()->bounded(200, 401);
    QTimer::singleShot(delay, [this]() {
        if (planActive)
            timer->start();
    });
}

#ifdef Q_OS_WIN
void PlanWidget::performKeyAction(int keyCode)
{
    INPUT inputDown = {};
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = static_cast<WORD>(keyCode);
    SendInput(1, &inputDown, sizeof(INPUT));
    int pressDelay = QRandomGenerator::global()->bounded(50, 151);
    QTimer::singleShot(pressDelay, [keyCode]() {
        INPUT inputUp = {};
        inputUp.type = INPUT_KEYBOARD;
        inputUp.ki.wVk = static_cast<WORD>(keyCode);
        inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &inputUp, sizeof(INPUT));        
        Beep(800, 100);
    });
}

void PlanWidget::performDisconnect()
{
    if (processName.isEmpty())
        return;
    std::wstring procName = processName.toStdWString();
    DisconnectNetworkConnections(procName);
    Beep(500, 100);
}
#endif

QJsonObject PlanWidget::getConfig() const
{
    QJsonObject obj;
    obj["planName"] = getPlanName();
    // Сохраняем трекинговые точки
    QTableWidget *table = findChild<QTableWidget*>("trackingTable");
    QJsonArray points;
    if(table) {
        for (int i = 0; i < table->rowCount(); i++) {
            QJsonObject point;
            point["x"] = table->item(i,0) ? table->item(i,0)->text() : "";
            point["y"] = table->item(i,1) ? table->item(i,1)->text() : "";
            point["color"] = table->item(i,2) ? table->item(i,2)->text() : "";
            QWidget *w = table->cellWidget(i,3);
            QCheckBox *cb = w ? w->findChild<QCheckBox*>() : nullptr;
            point["invert"] = cb ? cb->isChecked() : false;
            points.append(point);
        }
    }
    obj["trackingPoints"] = points;
    // Сохраняем действие
    QComboBox *actionComboBox = findChild<QComboBox*>("actionComboBox");
    QLineEdit *keyLineEdit = findChild<QLineEdit*>("keyLineEdit");
    if(actionComboBox && keyLineEdit) {
        obj["action"] = actionComboBox->currentText();
        obj["key"] = keyLineEdit->text();
    }
    return obj;
}

void PlanWidget::loadConfig(const QJsonObject &obj)
{
    if(obj.contains("planName"))
        setPlanName(obj["planName"].toString());
    // Загружаем трекинговые точки
    QTableWidget *table = findChild<QTableWidget*>("trackingTable");
    if(table) {
        table->setRowCount(0);
        QJsonArray points = obj["trackingPoints"].toArray();
        for (int i = 0; i < points.size(); i++) {
            QJsonObject point = points[i].toObject();
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(point["x"].toString()));
            table->setItem(row, 1, new QTableWidgetItem(point["y"].toString()));
            table->setItem(row, 2, new QTableWidgetItem(point["color"].toString()));
            QWidget *container = new QWidget(this);
            QHBoxLayout *layout = new QHBoxLayout(container);
            layout->setContentsMargins(0,0,0,0);
            layout->setAlignment(Qt::AlignCenter);
            QCheckBox *checkBox = new QCheckBox(container);
            checkBox->setChecked(point["invert"].toBool());
            layout->addWidget(checkBox);
            table->setCellWidget(row, 3, container);
        }
    }
    // Загружаем действие
    QComboBox *actionComboBox = findChild<QComboBox*>("actionComboBox");
    QLineEdit *keyLineEdit = findChild<QLineEdit*>("keyLineEdit");
    if(actionComboBox && keyLineEdit) {
        QString act = obj["action"].toString();
        int idx = actionComboBox->findText(act);
        if(idx != -1)
            actionComboBox->setCurrentIndex(idx);
        keyLineEdit->setText(obj["key"].toString());
    }
}