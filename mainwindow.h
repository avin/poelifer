#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTabWidget;
class QLineEdit;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void onAddPlan();
    void onRemovePlan();
    void onStartAllPlans();
    void onStopAllPlans();
    void onScanDisconnectKey();
    void onProcessNameChanged(const QString &text);
    void onSaveConfig();
    void onLoadConfig();
private:
    Ui::MainWindow *ui;
    QTabWidget *tabWidget;
    QLineEdit *processLineEdit;
    QLineEdit *disconnectKeyLineEdit;
};

#endif // MAINWINDOW_H