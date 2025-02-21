#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onToggleTracking();
    void checkConditions();
    void onSaveConfigClicked();
    void onLoadConfigClicked();
    void onAddPointClicked();
    void onRemovePointClicked();
    void onPickTrackingPointClicked();
    void onScanKeyClicked();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    bool trackingActive;

    void loadConfig();
    void saveConfig();
    QString getActiveProcessName();
    QColor getPixelColor(int x, int y);
};

#endif // MAINWINDOW_H