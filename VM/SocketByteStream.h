// SocketByteStream.h
#pragma once

#include <Windows.h>
#include <WinSock2.h>
#include <mfapi.h>
#include <wrl/client.h>
#include <shlwapi.h> 

#pragma comment(lib, "shlwapi.lib")

// ========================================================================
//                 共享的辅助函数 (使用 inline 避免重定义)
// ========================================================================

// 将数据完整地发送出去
inline bool SendFull(SOCKET sock, const char* data, int size) {
    if (sock == INVALID_SOCKET) return false;
    int totalSent = 0;
    while (totalSent < size) {
        int sentNow = send(sock, data + totalSent, size - totalSent, 0);
        if (sentNow == SOCKET_ERROR) return false;
        totalSent += sentNow;
    }
    return true;
}

// 接收指定大小的数据，直到收满为止
inline bool RecvFull(SOCKET sock, char* data, int size) {
    if (sock == INVALID_SOCKET) return false;
    int totalRecvd = 0;
    while (totalRecvd < size) {
        int recvdNow = recv(sock, data + totalRecvd, size - totalRecvd, 0);
        if (recvdNow <= 0) return false; // <= 0 表示错误或连接关闭
        totalRecvd += recvdNow;
    }
    return true;
}


// ========================================================================
//          SocketByteStream 类: 将 SOCKET 适配为 IMFByteStream
// ========================================================================

class SocketByteStream : public IMFByteStream {
public:
    static HRESULT Create(SOCKET socket, SocketByteStream** ppStream) {
        *ppStream = new (std::nothrow) SocketByteStream(socket);
        if (*ppStream == nullptr) {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    // --- IUnknown methods ---
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IMFByteStream) {
            *ppv = static_cast<IMFByteStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    // --- IMFByteStream methods ---
    STDMETHODIMP GetCapabilities(DWORD* pdwCapabilities) override {
        *pdwCapabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_WRITABLE;
        return S_OK;
    }

    STDMETHODIMP Read(BYTE* pb, ULONG cb, ULONG* pcbRead) override {
        if (!RecvFull(m_socket, (char*)pb, cb)) {
            if (pcbRead) *pcbRead = 0;
            return E_FAIL;
        }
        if (pcbRead) *pcbRead = cb;
        return S_OK;
    }

    STDMETHODIMP Write(const BYTE* pb, ULONG cb, ULONG* pcbWritten) override {
        if (!SendFull(m_socket, (const char*)pb, cb)) {
            if (pcbWritten) *pcbWritten = 0;
            return E_FAIL;
        }
        if (pcbWritten) *pcbWritten = cb;
        return S_OK;
    }

    // --- 未实现的 IMFByteStream 方法 (对于网络流不适用) ---
    STDMETHODIMP GetLength(QWORD* pqwLength) override { return E_NOTIMPL; }
    STDMETHODIMP SetLength(QWORD qwLength) override { return E_NOTIMPL; }
    STDMETHODIMP GetCurrentPosition(QWORD* pqwPosition) override { return E_NOTIMPL; }
    STDMETHODIMP SetCurrentPosition(QWORD qwPosition) override { return E_NOTIMPL; }
    STDMETHODIMP IsEndOfStream(BOOL* pfEndOfStream) override { *pfEndOfStream = FALSE; return S_OK; }
    STDMETHODIMP BeginRead(BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState) override { return E_NOTIMPL; }
    STDMETHODIMP EndRead(IMFAsyncResult* pResult, ULONG* pcbRead) override { return E_NOTIMPL; }
    STDMETHODIMP BeginWrite(const BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState) override { return E_NOTIMPL; }
    STDMETHODIMP EndWrite(IMFAsyncResult* pResult, ULONG* pcbWritten) override { return E_NOTIMPL; }
    STDMETHODIMP Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD* pqwCurrentPosition) override { return E_NOTIMPL; }
    STDMETHODIMP Flush() override { return S_OK; }
    STDMETHODIMP Close() override { return S_OK; }

private:
    SocketByteStream(SOCKET socket) : m_cRef(1), m_socket(socket) {}
    ~SocketByteStream() {}

    volatile ULONG m_cRef;
    SOCKET m_socket;
};