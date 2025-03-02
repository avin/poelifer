#ifndef PLANWIDGET_H
#define PLANWIDGET_H

#include <QWidget>
#include <QString>
#include <QJsonObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class QTimer;
class QLineEdit;

class PlanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlanWidget(QWidget *parent = nullptr);
    ~PlanWidget();
    void setProcessName(const QString &processName);
    QString getProcessName() const;
    void setPlanName(const QString &name);
    QString getPlanName() const;
    bool isPlanActive() const;
    void startPlan();
    void stopPlan();
    QJsonObject getConfig() const;
    void loadConfig(const QJsonObject &obj);
signals:
    void statusChanged(bool running);
    void planNameChanged(const QString &name);
private slots:
    void onTogglePlan();
    void onAddPoint();
    void onRemovePoint();
    void onPickPoint();
    void onScanKey();
    void checkConditions();
private:
    QTimer *timer;
    bool planActive;
    QString processName;
    // Используем для хранения имени плана
    QLineEdit *planNameLineEdit;
    bool arePixelConditionsMet();
#ifdef Q_OS_WIN
    void performKeyAction(int keyCode);
    void performDisconnect();
#endif
};

#endif // PLANWIDGET_H