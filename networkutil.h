#ifndef NETWORKUTIL_H
#define NETWORKUTIL_H

#include <vector>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#endif

std::vector<DWORD> GetProcessIds(const std::wstring& processName);
void DisconnectNetworkConnections(const std::wstring& processName);

#endif // NETWORKUTIL_H