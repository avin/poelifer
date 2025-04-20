#include "planwidget.h"
#include "keycapturedialog.h"
#include "networkutil.h"
#include "screenpicker.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScreen>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QMap>
#include <QList>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

PlanWidget::PlanWidget(QWidget *parent)
    : QWidget(parent)
    , planActive(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Инпут для имени плана
    planNameLineEdit = new QLineEdit(this);
    planNameLineEdit->setPlaceholderText("Имя плана");
    planNameLineEdit->setObjectName("planNameLineEdit");
    mainLayout->addWidget(planNameLineEdit);
    connect(planNameLineEdit, &QLineEdit::textChanged, this, [this](const QString &name) {
        emit planNameChanged(name);
    });

    // Группа точек отслеживания
    QGroupBox *groupTracking = new QGroupBox("Точки отслеживания", this);
    QVBoxLayout *trackingLayout = new QVBoxLayout(groupTracking);
    QTableWidget *table = new QTableWidget(0, 6, this);
    table->setHorizontalHeaderLabels(QStringList() << "X" << "Y" << "Color" << "Tolerance (%)"
                                                   << "Not eq" << "Group");
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

PlanWidget::~PlanWidget() {}

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
    if (planNameLineEdit)
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
    QPushButton *toggleButton = findChild<QPushButton *>("toggleButton");
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
    QTableWidget *table = findChild<QTableWidget *>("trackingTable");
    if (!table)
        return;
    int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem("0"));
    table->setItem(row, 1, new QTableWidgetItem("0"));
    table->setItem(row, 2, new QTableWidgetItem("#FFFFFF"));
    table->setItem(row, 3, new QTableWidgetItem("0"));
    QWidget *container = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);
    QCheckBox *checkBox = new QCheckBox(container);
    layout->addWidget(checkBox);
    table->setCellWidget(row, 4, container);
    table->setItem(row, 5, new QTableWidgetItem(""));
}

void PlanWidget::onRemovePoint()
{
    QTableWidget *table = findChild<QTableWidget *>("trackingTable");
    if (!table)
        return;
    QList<QTableWidgetItem *> selected = table->selectedItems();
    if (selected.isEmpty())
        return;
    int row = selected.first()->row();
    table->removeRow(row);
}

void PlanWidget::onPickPoint()
{
    QTableWidget *table = findChild<QTableWidget *>("trackingTable");
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
        table->setItem(row, 3, new QTableWidgetItem("0"));
        QWidget *container = new QWidget(this);
        QHBoxLayout *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setAlignment(Qt::AlignCenter);
        QCheckBox *checkBox = new QCheckBox(container);
        layout->addWidget(checkBox);
        table->setCellWidget(row, 4, container);
        table->setItem(row, 5, new QTableWidgetItem(""));
    });
    picker->show();
}

void PlanWidget::onScanKey()
{
    KeyCaptureDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QLineEdit *keyLineEdit = findChild<QLineEdit *>("keyLineEdit");
        if (keyLineEdit) {
            keyLineEdit->setText(QString::number(dialog.capturedKey()));
        }
    }
}

bool PlanWidget::arePixelConditionsMet()
{
    QTableWidget *table = findChild<QTableWidget *>("trackingTable");
    if (!table)
        return false;
    int rowCount = table->rowCount();
    if (rowCount == 0)
        return false;

    QMap<QString, QList<bool>> groupResults;
    for (int i = 0; i < rowCount; i++) {
        QTableWidgetItem *xItem = table->item(i, 0);
        QTableWidgetItem *yItem = table->item(i, 1);
        QTableWidgetItem *colorItem = table->item(i, 2);
        QTableWidgetItem *tolItem = table->item(i, 3);
        if (!xItem || !yItem || !colorItem || !tolItem)
            continue;
        bool okX, okY, okTol;
        int x = xItem->text().toInt(&okX);
        int y = yItem->text().toInt(&okY);
        double tolPercent = tolItem->text().toDouble(&okTol);
        if (!okX || !okY || !okTol)
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
        int dr = abs(currentColor.red() - expectedColor.red());
        int dg = abs(currentColor.green() - expectedColor.green());
        int db = abs(currentColor.blue() - expectedColor.blue());
        double tol = tolPercent / 100.0 * 255.0;
        bool match = (dr <= tol && dg <= tol && db <= tol);

        QTableWidgetItem *groupItem = table->item(i, 5);
        QString groupId = groupItem ? groupItem->text().trimmed() : "";

        QWidget *widget = table->cellWidget(i, 4);
        QCheckBox *checkBox = widget ? widget->findChild<QCheckBox *>() : nullptr;
        bool invertFlag = (checkBox && checkBox->isChecked());
        bool pixelCondition = invertFlag ? !match : match;

        if (groupId.isEmpty()) {
            // обязательная проверка
            if (!pixelCondition)
                return false;
        } else {
            // накапливаем результаты по группам
            groupResults[groupId].append(pixelCondition);
        }
    }
    // проверяем опциональные группы: требуется, чтобы ВСЕ пиксели в группе НЕ прошли проверку
    for (auto it = groupResults.constBegin(); it != groupResults.constEnd(); ++it) {
        const QList<bool> &results = it.value();
        bool allFailed = true;
        for (bool res : results) {
            if (res) {
                allFailed = false;
                break;
            }
        }
        if (!allFailed)
            return false;
    }
    return true;
}

void PlanWidget::checkConditions()
{
    if (!planActive)
        return;
    if (!arePixelConditionsMet())
        return;
    QComboBox *actionComboBox = findChild<QComboBox *>("actionComboBox");
    QLineEdit *keyLineEdit = findChild<QLineEdit *>("keyLineEdit");
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
    QTableWidget *table = findChild<QTableWidget *>("trackingTable");
    QJsonArray points;
    if (table) {
        for (int i = 0; i < table->rowCount(); i++) {
            QJsonObject point;
            point["x"] = table->item(i, 0) ? table->item(i, 0)->text() : "";
            point["y"] = table->item(i, 1) ? table->item(i, 1)->text() : "";
            point["color"] = table->item(i, 2) ? table->item(i, 2)->text() : "";
            point["tolerance"] = table->item(i, 3) ? table->item(i, 3)->text() : "0";
            QWidget *w = table->cellWidget(i, 4);
            QCheckBox *cb = w ? w->findChild<QCheckBox *>() : nullptr;
            point["invert"] = cb ? cb->isChecked() : false;
            QTableWidgetItem *groupItem = table->item(i, 5);
            point["groupId"] = groupItem ? groupItem->text() : "";
            points.append(point);
        }
    }
    obj["trackingPoints"] = points;
    // Сохраняем действие
    QComboBox *actionComboBox = findChild<QComboBox *>("actionComboBox");
    QLineEdit *keyLineEdit = findChild<QLineEdit *>("keyLineEdit");
    if (actionComboBox && keyLineEdit) {
        obj["action"] = actionComboBox->currentText();
        obj["key"] = keyLineEdit->text();
    }
    return obj;
}

void PlanWidget::loadConfig(const QJsonObject &obj)
{
    if (obj.contains("planName"))
        setPlanName(obj["planName"].toString());
    // Загружаем трекинговые точки
    QTableWidget *table = findChild<QTableWidget *>("trackingTable");
    if (table) {
        table->setRowCount(0);
        QJsonArray points = obj["trackingPoints"].toArray();
        for (int i = 0; i < points.size(); i++) {
            QJsonObject point = points[i].toObject();
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(point["x"].toString()));
            table->setItem(row, 1, new QTableWidgetItem(point["y"].toString()));
            table->setItem(row, 2, new QTableWidgetItem(point["color"].toString()));
            QString tol = point.contains("tolerance") ? point["tolerance"].toString() : "0";
            table->setItem(row, 3, new QTableWidgetItem(tol));
            QWidget *container = new QWidget(this);
            QHBoxLayout *layout = new QHBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignCenter);
            QCheckBox *checkBox = new QCheckBox(container);
            checkBox->setChecked(point["invert"].toBool());
            layout->addWidget(checkBox);
            table->setCellWidget(row, 4, container);
            QString grp = point.contains("groupId") ? point["groupId"].toString() : "";
            table->setItem(row, 5, new QTableWidgetItem(grp));
        }
    }
    // Загружаем действие
    QComboBox *actionComboBox = findChild<QComboBox *>("actionComboBox");
    QLineEdit *keyLineEdit = findChild<QLineEdit *>("keyLineEdit");
    if (actionComboBox && keyLineEdit) {
        QString act = obj["action"].toString();
        int idx = actionComboBox->findText(act);
        if (idx != -1)
            actionComboBox->setCurrentIndex(idx);
        keyLineEdit->setText(obj["key"].toString());
    }
}
