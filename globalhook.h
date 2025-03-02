#ifndef GLOBALHOOK_H
#define GLOBALHOOK_H

#include <QObject>
#include <windows.h>
#include <string>

class GlobalHook : public QObject
{
    Q_OBJECT
public:
    static GlobalHook* instance();
    void start();
    void stop();
    void setDisconnectKey(int key);
    void setProcessName(const std::wstring &processName);
signals:
    void disconnectTriggered();
private:
    explicit GlobalHook(QObject *parent = nullptr);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    HHOOK m_hook;
    int m_disconnectKey;
    std::wstring m_processName;
};

#endif // GLOBALHOOK_H