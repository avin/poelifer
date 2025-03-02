#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "planwidget.h"
#include "networkutil.h"
#include "keycapturedialog.h"
#include "globalhook.h"

#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QMessageBox>
#include <QIntValidator>
#include <QFileDialog>
#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);    

    // Глобальные настройки с разделением по строкам
    QWidget *globalWidget = new QWidget(this);
    QVBoxLayout *globalLayout = new QVBoxLayout(globalWidget);

    // Первый ряд: кнопки сохранения и загрузки конфига
    QHBoxLayout *configButtonsLayout = new QHBoxLayout();
    QPushButton *saveConfigButton = new QPushButton("Сохранить конфиг", this);
    QPushButton *loadConfigButton = new QPushButton("Загрузить конфиг", this);
    configButtonsLayout->addWidget(saveConfigButton);
    configButtonsLayout->addWidget(loadConfigButton);
    globalLayout->addLayout(configButtonsLayout);
    connect(saveConfigButton, &QPushButton::clicked, this, &MainWindow::onSaveConfig);
    connect(loadConfigButton, &QPushButton::clicked, this, &MainWindow::onLoadConfig);

    // Второй ряд: настройка имени процесса
    QHBoxLayout *processLayout = new QHBoxLayout();
    QLabel *processLabel = new QLabel("Название процесса:", this);
    processLayout->addWidget(processLabel);
    processLineEdit = new QLineEdit(this);
    processLineEdit->setPlaceholderText("Введите название процесса");
    processLineEdit->setObjectName("processLineEdit");
    processLayout->addWidget(processLineEdit);
    globalLayout->addLayout(processLayout);

    // Третий ряд: настройка клавиши для дисконнекта
    QHBoxLayout *disconnectLayout = new QHBoxLayout();
    QLabel *disconnectLabel = new QLabel("Код клавиши для дисконнекта:", this);
    disconnectLayout->addWidget(disconnectLabel);
    disconnectKeyLineEdit = new QLineEdit(this);
    disconnectKeyLineEdit->setPlaceholderText("Введите код клавиши");
    disconnectKeyLineEdit->setValidator(new QIntValidator(0, 255, this));
    disconnectKeyLineEdit->setObjectName("disconnectKeyLineEdit");
    disconnectLayout->addWidget(disconnectKeyLineEdit);
    QPushButton *scanDisconnectKeyButton = new QPushButton("Сканировать", this);
    disconnectLayout->addWidget(scanDisconnectKeyButton);
    globalLayout->addLayout(disconnectLayout);
    connect(scanDisconnectKeyButton, &QPushButton::clicked, this, &MainWindow::onScanDisconnectKey);

    // Создаем QTabWidget для планов
    tabWidget = new QTabWidget(this);

    // Панель управления планами
    QPushButton *addPlanButton = new QPushButton("Добавить план", this);
    QPushButton *removePlanButton = new QPushButton("Удалить план", this);
    QPushButton *startAllButton = new QPushButton("Запустить все", this);
    QPushButton *stopAllButton = new QPushButton("Остановить все", this);

    connect(addPlanButton, &QPushButton::clicked, this, &MainWindow::onAddPlan);
    connect(removePlanButton, &QPushButton::clicked, this, &MainWindow::onRemovePlan);
    connect(startAllButton, &QPushButton::clicked, this, &MainWindow::onStartAllPlans);
    connect(stopAllButton, &QPushButton::clicked, this, &MainWindow::onStopAllPlans);

    // Общий макет окна
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->addWidget(globalWidget);
    mainLayout->addWidget(tabWidget);
    QHBoxLayout *plansLayout = new QHBoxLayout();
    plansLayout->addWidget(addPlanButton);
    plansLayout->addWidget(removePlanButton);
    plansLayout->addWidget(startAllButton);
    plansLayout->addWidget(stopAllButton);
    mainLayout->addLayout(plansLayout);
    setCentralWidget(central);

    connect(processLineEdit, &QLineEdit::textChanged, this, &MainWindow::onProcessNameChanged);
    // Обновляем глобальный хук при изменении кода клавиши для дисконнекта
    connect(disconnectKeyLineEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        bool ok;
        int key = text.toInt(&ok, 10);
        if(ok)
            GlobalHook::instance()->setDisconnectKey(key);
    });

    // Устанавливаем имя процесса в глобальном хуке
    connect(processLineEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        GlobalHook::instance()->setProcessName(text.toStdWString());
    });

    // Запускаем глобальный хук
    GlobalHook::instance()->start();

    // Попытка загрузить последний конфиг
    QSettings settings("MyCompany", "PoeLifer");
    QString lastConfig = settings.value("lastConfigPath", "").toString();
    if (!lastConfig.isEmpty()) {
        QFile file(lastConfig);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                // Устанавливаем глобальные настройки
                QJsonObject globalObj = obj.value("global").toObject();
                processLineEdit->setText(globalObj.value("processName").toString());
                disconnectKeyLineEdit->setText(globalObj.value("disconnectKey").toString());
                // Загружаем планы
                QJsonArray plansArray = obj.value("plans").toArray();
                for (const QJsonValue &val : plansArray) {
                    QJsonObject planObj = val.toObject();
                    PlanWidget *plan = new PlanWidget(this);
                    plan->loadConfig(planObj);
                    plan->setProcessName(processLineEdit->text());
                    int index = tabWidget->addTab(plan, QString("%1 %2")
                                                   .arg(plan->getPlanName().isEmpty() ? "Без имени" : plan->getPlanName())
                                                   .arg(plan->isPlanActive() ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴")));
                    connect(plan, &PlanWidget::statusChanged, this, [=](bool running){
                        int idx = tabWidget->indexOf(plan);
                        if(idx >= 0) {
                            QString name = plan->getPlanName().isEmpty() ? "Без имени" : plan->getPlanName();
                            QString statusEmoji = running ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴");
                            tabWidget->setTabText(idx, QString("%1 %2").arg(name).arg(statusEmoji));
                        }
                    });
                    connect(plan, &PlanWidget::planNameChanged, this, [=](const QString &name){
                        int idx = tabWidget->indexOf(plan);
                        if(idx >= 0) {
                            QString statusEmoji = plan->isPlanActive() ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴");
                            tabWidget->setTabText(idx, QString("%1 %2").arg(name.isEmpty() ? "Без имени" : name).arg(statusEmoji));
                        }
                    });
                }
            }
        }
    } else {
        onAddPlan();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAddPlan()
{
    PlanWidget *plan = new PlanWidget(this);
    plan->setProcessName(processLineEdit->text());
    int index = tabWidget->addTab(plan, QString("Без имени 🔴"));
    tabWidget->setCurrentIndex(index);
    connect(plan, &PlanWidget::statusChanged, this, [=](bool running){
        int idx = tabWidget->indexOf(plan);
        if(idx >= 0) {
            QString name = plan->getPlanName().isEmpty() ? "Без имени" : plan->getPlanName();
            QString statusEmoji = running ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴");
            tabWidget->setTabText(idx, QString("%1 %2").arg(name).arg(statusEmoji));
        }
    });
    connect(plan, &PlanWidget::planNameChanged, this, [=](const QString &name){
        int idx = tabWidget->indexOf(plan);
        if(idx >= 0) {
            QString statusEmoji = plan->isPlanActive() ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴");
            tabWidget->setTabText(idx, QString("%1 %2").arg(name.isEmpty() ? "Без имени" : name).arg(statusEmoji));
        }
    });
}

void MainWindow::onRemovePlan()
{
    int index = tabWidget->currentIndex();
    if (index < 0)
        return;
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение", "Вы уверены, что хотите удалить выбранный план?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No)
        return;
    QWidget *widget = tabWidget->widget(index);
    tabWidget->removeTab(index);
    widget->deleteLater();
}

void MainWindow::onStartAllPlans()
{
    for (int i = 0; i < tabWidget->count(); i++) {
        PlanWidget *plan = qobject_cast<PlanWidget*>(tabWidget->widget(i));
        if(plan) {
            plan->startPlan();
        }
    }
}

void MainWindow::onStopAllPlans()
{
    for (int i = 0; i < tabWidget->count(); i++) {
        PlanWidget *plan = qobject_cast<PlanWidget*>(tabWidget->widget(i));
        if(plan) {
            plan->stopPlan();
        }
    }
}

void MainWindow::onScanDisconnectKey()
{
    KeyCaptureDialog dialog(this);
    if(dialog.exec() == QDialog::Accepted) {
         disconnectKeyLineEdit->setText(QString::number(dialog.capturedKey()));
    }
}

void MainWindow::onProcessNameChanged(const QString &text)
{
    for (int i = 0; i < tabWidget->count(); i++) {
        PlanWidget *plan = qobject_cast<PlanWidget*>(tabWidget->widget(i));
        if (plan) {
            plan->setProcessName(text);
        }
    }
    GlobalHook::instance()->setProcessName(text.toStdWString());
}

void MainWindow::onSaveConfig()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить конфиг", "", "JSON Files (*.json)");
    if (fileName.isEmpty())
        return;
    QJsonObject rootObj;
    QJsonObject globalObj;
    globalObj["processName"] = processLineEdit->text();
    globalObj["disconnectKey"] = disconnectKeyLineEdit->text();
    rootObj["global"] = globalObj;
    QJsonArray plansArray;
    for (int i = 0; i < tabWidget->count(); i++) {
        PlanWidget *plan = qobject_cast<PlanWidget*>(tabWidget->widget(i));
        if (plan) {
            plansArray.append(plan->getConfig());
        }
    }
    rootObj["plans"] = plansArray;
    QJsonDocument doc(rootObj);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QSettings settings("MyCompany", "PoeLifer");
        settings.setValue("lastConfigPath", fileName);
        QMessageBox::information(this, "Конфигурация", "Конфигурация сохранена");
    }
}

void MainWindow::onLoadConfig()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Загрузить конфиг", "", "JSON Files (*.json)");
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;
    QJsonObject rootObj = doc.object();
    QJsonObject globalObj = rootObj.value("global").toObject();
    processLineEdit->setText(globalObj.value("processName").toString());
    disconnectKeyLineEdit->setText(globalObj.value("disconnectKey").toString());
    // Очистить существующие планы
    while (tabWidget->count() > 0) {
        QWidget *w = tabWidget->widget(0);
        tabWidget->removeTab(0);
        w->deleteLater();
    }
    QJsonArray plansArray = rootObj.value("plans").toArray();
    for (const QJsonValue &val : plansArray) {
        QJsonObject planObj = val.toObject();
        PlanWidget *plan = new PlanWidget(this);
        plan->loadConfig(planObj);
        plan->setProcessName(processLineEdit->text());
        int index = tabWidget->addTab(plan, QString("%1 %2")
                                           .arg(plan->getPlanName().isEmpty() ? "Без имени" : plan->getPlanName())
                                           .arg(plan->isPlanActive() ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴")));
        connect(plan, &PlanWidget::statusChanged, this, [=](bool running){
            int idx = tabWidget->indexOf(plan);
            if(idx >= 0) {
                QString name = plan->getPlanName().isEmpty() ? "Без имени" : plan->getPlanName();
                QString statusEmoji = running ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴");
                tabWidget->setTabText(idx, QString("%1 %2").arg(name).arg(statusEmoji));
            }
        });
        connect(plan, &PlanWidget::planNameChanged, this, [=](const QString &name){
            int idx = tabWidget->indexOf(plan);
            if(idx >= 0) {
                QString statusEmoji = plan->isPlanActive() ? QString::fromUtf8("🟢") : QString::fromUtf8("🔴");
                tabWidget->setTabText(idx, QString("%1 %2").arg(name.isEmpty() ? "Без имени" : name).arg(statusEmoji));
            }
        });
    }
    QSettings settings("MyCompany", "PoeLifer");
    settings.setValue("lastConfigPath", fileName);
    QMessageBox::information(this, "Конфигурация", "Конфигурация загружена");
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
}
