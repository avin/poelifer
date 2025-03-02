#include "globalhook.h"
#include "networkutil.h"
#include <windows.h>
#include <psapi.h>
#include <vector>
#include <algorithm>
#include <cstdio>

GlobalHook* GlobalHook::instance()
{
    static GlobalHook inst;
    return &inst;
}

GlobalHook::GlobalHook(QObject *parent)
    : QObject(parent), m_hook(nullptr), m_disconnectKey(0)
{
}

void GlobalHook::start()
{
    if (!m_hook)
    {
        m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    }
}

void GlobalHook::stop()
{
    if (m_hook)
    {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
}

void GlobalHook::setDisconnectKey(int key)
{
    m_disconnectKey = key;
}

void GlobalHook::setProcessName(const std::wstring &processName)
{
    m_processName = processName;
}

LRESULT CALLBACK GlobalHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        GlobalHook* inst = GlobalHook::instance();
        // Если нажатая клавиша совпадает с клавишей для дисконнекта...
        if(pKeyboard->vkCode == static_cast<DWORD>(inst->m_disconnectKey))
        {
            // Проверяем, что активное окно принадлежит указанному процессу.
            HWND hwndActive = GetForegroundWindow();
            if (hwndActive)
            {
                DWORD activePid = 0;
                GetWindowThreadProcessId(hwndActive, &activePid);
                if(activePid != 0)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, activePid);
                    if(hProcess)
                    {
                        HMODULE hMod;
                        DWORD cbNeeded;
                        if(EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
                        {
                            TCHAR activeProcName[MAX_PATH] = {0};
                            if(GetModuleBaseName(hProcess, hMod, activeProcName, MAX_PATH))
                            {
                                std::wstring activeName(activeProcName);
                                // Сравнение имен процессов (без учета регистра)
                                if(_wcsicmp(activeName.c_str(), inst->m_processName.c_str()) == 0)
                                {
                                    // Используем уникальный звук для глобального дисконнекта                                    
                                    DisconnectNetworkConnections(inst->m_processName);                                    
                                    emit inst->disconnectTriggered();
                                    Beep(800, 100);
                                }
                            }
                        }
                        CloseHandle(hProcess);
                    }
                }
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
