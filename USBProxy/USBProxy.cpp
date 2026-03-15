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
#include <atomic>
#include <iomanip>

// Removed the silent window pragma to show the console
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Ole32.lib")

#define PROXY_BUF_SIZE (512 * 1024)

#ifndef SIO_TCP_SET_ACK_FREQUENCY
#define SIO_TCP_SET_ACK_FREQUENCY _WSAIOW(IOC_VENDOR, 23)
#endif

struct SOCKADDR_HV { ADDRESS_FAMILY Family; USHORT Reserved; GUID VmId; GUID ServiceId; };
DEFINE_GUID(HV_GUID_ZERO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

static const wchar_t* SERVICE_ID = L"{45784879-7065-7256-5553-4250726F7879}";

// Atomic counters for throughput
std::atomic<long long> g_stat_up(0);   // Guest to Host
std::atomic<long long> g_stat_down(0); // Host to Guest

void SpeedMonitor() {
    long long last_up = 0, last_down = 0;
    while (true) {
        Sleep(1000);
        long long curr_up = g_stat_up.load();
        long long curr_down = g_stat_down.load();

        double up_kb = (curr_up - last_up) / 1024.0;
        double down_kb = (curr_down - last_down) / 1024.0;

        last_up = curr_up;
        last_down = curr_down;

        // \r keeps the output on one line
        std::cout << "\r[Transfer] Up: " << std::fixed << std::setprecision(2) << up_kb << " KB/s | "
            << "Down: " << down_kb << " KB/s    " << std::flush;
    }
}

void DataPump(SOCKET s_src, SOCKET s_dst, std::atomic<long long>& counter) {
    void* buffer = _aligned_malloc(PROXY_BUF_SIZE, 4096);
    while (true) {
        int b = recv(s_src, (char*)buffer, PROXY_BUF_SIZE, 0);
        if (b <= 0) break;
        int s = send(s_dst, (char*)buffer, b, 0);
        if (s <= 0) break;
        counter += s;
    }
    _aligned_free(buffer);
    closesocket(s_src);
    closesocket(s_dst);
}

void ExecuteUsbIpAttach(std::string busId) {
    Sleep(500);
    std::string cmd = "usbip.exe attach -r 127.0.0.1 -b " + busId;
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
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
    std::cout << "\n[Event] Device Received: " << busId << std::endl;

    SOCKET tcpListen = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sa_tcp = { AF_INET, htons(3240) };
    sa_tcp.sin_addr.s_addr = inet_addr("127.0.0.1");

    int opt = 1;
    setsockopt(tcpListen, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (bind(tcpListen, (sockaddr*)&sa_tcp, sizeof(sa_tcp)) == SOCKET_ERROR) {
        std::cout << "[Error] Port 3240 bind failed." << std::endl;
        closesocket(hvClient); return;
    }
    listen(tcpListen, 1);

    std::thread(ExecuteUsbIpAttach, busId).detach();
    SOCKET tcpClient = accept(tcpListen, NULL, NULL);
    closesocket(tcpListen);

    if (tcpClient != INVALID_SOCKET) {
        std::cout << "[Status] Tunnel Established." << std::endl;

        int nodelay = 1;
        int microBuf = 8192;
        int ackFreq = 1;
        DWORD bytes = 0;

        WSAIoctl(tcpClient, SIO_TCP_SET_ACK_FREQUENCY, &ackFreq, sizeof(ackFreq), NULL, 0, &bytes, NULL, NULL);
        setsockopt(tcpClient, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
        setsockopt(tcpClient, SOL_SOCKET, SO_SNDBUF, (char*)&microBuf, sizeof(microBuf));
        setsockopt(tcpClient, SOL_SOCKET, SO_RCVBUF, (char*)&microBuf, sizeof(microBuf));

        // Start Upstream and Downstream pumps
        std::thread(DataPump, hvClient, tcpClient, std::ref(g_stat_down)).detach();
        DataPump(tcpClient, hvClient, std::ref(g_stat_up));

        std::cout << "\n[Status] Session Closed: " << busId << std::endl;
    }
    else {
        closesocket(hvClient);
    }
}

int main() {
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

    std::cout << "Hyper-V USBIP Guest Proxy" << std::endl;
    std::cout << "-----------------------------------" << std::endl;

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET hvListen = socket(34, SOCK_STREAM, 1);
    if (hvListen == INVALID_SOCKET) {
        std::cout << "[Error] Failed to create HV socket." << std::endl;
        return 1;
    }

    SOCKADDR_HV sa_listen = { 34, 0, HV_GUID_ZERO };
    CLSIDFromString(SERVICE_ID, &sa_listen.ServiceId);

    if (bind(hvListen, (const sockaddr*)&sa_listen, sizeof(sa_listen)) != SOCKET_ERROR) {
        listen(hvListen, SOMAXCONN);
        std::cout << "[Status] Ready. Waiting for host device..." << std::endl;

        // Start the global speed monitor thread
        std::thread(SpeedMonitor).detach();

        while (true) {
            SOCKET hvClient = accept(hvListen, NULL, NULL);
            if (hvClient != INVALID_SOCKET) {
                std::thread(HandleAutoSession, hvClient).detach();
            }
        }
    }
    else {
        std::cout << "[Error] Socket bind failed. Check GUID or Permissions." << std::endl;
    }

    WSACleanup();
    return 0;
}