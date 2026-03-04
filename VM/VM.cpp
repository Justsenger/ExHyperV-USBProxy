#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <combaseapi.h>
#include <malloc.h>
#include <thread>
#include <string>
#include <iostream>

// 强制链接为 Windows 子系统
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Ole32.lib")

#define PROXY_BUF_SIZE (512 * 1024)

// 手动定义 ACK 频率控制码
#ifndef SIO_TCP_SET_ACK_FREQUENCY
#define SIO_TCP_SET_ACK_FREQUENCY _WSAIOW(IOC_VENDOR, 23)
#endif

struct SOCKADDR_HV { ADDRESS_FAMILY Family; USHORT Reserved; GUID VmId; GUID ServiceId; };
DEFINE_GUID(HV_GUID_ZERO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

static const wchar_t* SERVICE_ID = L"{45784879-7065-7256-5553-4250726F7879}";

void DataPump(SOCKET s1, SOCKET s2) {
    void* buffer = _aligned_malloc(PROXY_BUF_SIZE, 4096);
    while (true) {
        int b = recv(s1, (char*)buffer, PROXY_BUF_SIZE, 0);
        if (b <= 0) break;
        if (send(s2, (char*)buffer, b, 0) <= 0) break;
    }
    _aligned_free(buffer);
    closesocket(s1); closesocket(s2);
}

void ExecuteUsbIpAttach(std::string busId) {
    Sleep(500);
    std::string cmd = "usbip.exe attach -r 127.0.0.1 -b " + busId;
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
}

void HandleAutoSession(SOCKET hvClient) {

    char busIdBuf[64] = { 0 };
    int len = recv(hvClient, busIdBuf, 63, 0);
    if (len <= 0) { closesocket(hvClient); return; }

    std::string busId = busIdBuf;
    SOCKET tcpListen = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sa_tcp = { AF_INET, htons(3240) };
    sa_tcp.sin_addr.s_addr = inet_addr("127.0.0.1");

    int opt = 1;
    setsockopt(tcpListen, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (bind(tcpListen, (sockaddr*)&sa_tcp, sizeof(sa_tcp)) == SOCKET_ERROR) {
        closesocket(hvClient); return;
    }
    listen(tcpListen, 1);

    std::thread(ExecuteUsbIpAttach, busId).detach();
    SOCKET tcpClient = accept(tcpListen, NULL, NULL);
    closesocket(tcpListen);

    if (tcpClient != INVALID_SOCKET) {
        // 【参数配置】
        int nodelay = 1;
        int microBuf = 8192; // 8KB: 兼顾速度与稳定性的黄金值
        int ackFreq = 1;     // 强制立即 ACK，消除延迟
        DWORD bytes = 0;

        WSAIoctl(tcpClient, SIO_TCP_SET_ACK_FREQUENCY, &ackFreq, sizeof(ackFreq), NULL, 0, &bytes, NULL, NULL);

        setsockopt(tcpClient, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
        setsockopt(tcpClient, SOL_SOCKET, SO_SNDBUF, (char*)&microBuf, sizeof(microBuf));
        setsockopt(tcpClient, SOL_SOCKET, SO_RCVBUF, (char*)&microBuf, sizeof(microBuf));

        std::thread(DataPump, hvClient, tcpClient).detach();
        DataPump(tcpClient, hvClient);
    }
    else {
        closesocket(hvClient);
    }
}

int main() {
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET hvListen = socket(34, SOCK_STREAM, 1);
    SOCKADDR_HV sa_listen = { 34, 0, HV_GUID_ZERO };
    CLSIDFromString(SERVICE_ID, &sa_listen.ServiceId);

    if (bind(hvListen, (const sockaddr*)&sa_listen, sizeof(sa_listen)) != SOCKET_ERROR) {
        listen(hvListen, SOMAXCONN);
        while (true) {
            SOCKET hvClient = accept(hvListen, NULL, NULL);
            if (hvClient != INVALID_SOCKET) {
                std::thread(HandleAutoSession, hvClient).detach();
            }
        }
    }
    return 0;
}