#if defined(_WIN32)
#include "networkutil.h"
#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <algorithm>
#include <cstdlib>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

std::vector<DWORD> GetProcessIds(const std::wstring& processName)
{
    std::vector<DWORD> pids;
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return pids;
    cProcesses = cbNeeded / sizeof(DWORD);
    for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
            if (hProcess) {
                TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
                HMODULE hMod;
                DWORD cbNeededModule;
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeededModule)) {
                    GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
                }
                std::wstring currentProcessName = szProcessName;
                if (_wcsicmp(currentProcessName.c_str(), processName.c_str()) == 0) {
                    pids.push_back(aProcesses[i]);
                }
                CloseHandle(hProcess);
            }
        }
    }
    return pids;
}

void DisconnectNetworkConnections(const std::wstring& processName)
{
    std::vector<DWORD> pids = GetProcessIds(processName);
    if(pids.empty())
        return;
    
    PMIB_TCPTABLE_OWNER_PID pTcpTable = nullptr;
    DWORD dwSize = 0;
    DWORD dwResult = GetExtendedTcpTable(nullptr, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(dwSize);
    if(pTcpTable == nullptr)
        return;
    dwResult = GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if(dwResult == NO_ERROR)
    {
        for(DWORD i = 0; i < pTcpTable->dwNumEntries; i++)
        {
            MIB_TCPROW_OWNER_PID &row = pTcpTable->table[i];
            if(std::find(pids.begin(), pids.end(), row.dwOwningPid) != pids.end())
            {
                if(row.dwState == MIB_TCP_STATE_ESTAB ||
                   row.dwState == MIB_TCP_STATE_SYN_SENT ||
                   row.dwState == MIB_TCP_STATE_SYN_RCVD)
                {
                    MIB_TCPROW tcpRow;
                    tcpRow.dwLocalAddr = row.dwLocalAddr;
                    tcpRow.dwLocalPort = row.dwLocalPort;
                    tcpRow.dwRemoteAddr = row.dwRemoteAddr;
                    tcpRow.dwRemotePort = row.dwRemotePort;
                    tcpRow.dwState = MIB_TCP_STATE_DELETE_TCB;
                    SetTcpEntry(&tcpRow);
                }
            }
        }
    }
    free(pTcpTable);
}
#else
// Dummy implementations for non-Windows platforms.
std::vector<DWORD> GetProcessIds(const std::wstring& processName) { return {}; }
void DisconnectNetworkConnections(const std::wstring& processName) {}
#endif