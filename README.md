# ExHyperV-USBProxy

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)]()
[![Project](https://img.shields.io/badge/Part%20of-ExHyperV-orange.svg)](https://github.com/Justsenger/ExHyperV)

`ExHyperV-USBProxy` 是 **[ExHyperV](https://github.com/Justsenger/ExHyperV)** 项目的核心依赖组件之一。它主要用于实现 USBIP 协议在 Hyper-V 环境下的高性能传输。

## 📖 项目简介

在 Hyper-V 虚拟机中使用 USB 设备时，传统的网络方案往往受限于虚拟网卡的性能或配置复杂性。本项目作为“一阶方案”的代理工具，通过将标准的 **TCP/IP (USBIP)** 数据包封装进 **AF_HYPERV (Hyper-V Sockets)** 进行传输。

### 为什么选择 AF-HYPERV？
- **高性能**：绕过传统的网络协议栈，减少封装开销。
- **低延迟**：Host 与 Guest 之间直接通过 VMBus 通信。
- **零配置**：无需为虚拟机配置物理 IP 地址或 NAT 转发即可建立连接。

## 功能特性

- **协议转换**：将 USBIP 的 TCP 数据流无缝桥接到 AF_HYPERV。
- **高性能传输**：针对 Hyper-V 环境优化的数据转发逻辑。
- **轻量化**：C++ 编写，无冗余依赖，运行开销极低。
- **配套支持**：作为 ExHyperV 的核心组件，支持自动化挂载流程。

### 依赖环境
- Windows 10/11 或 Windows Server 2016+

### 安装与运行
在 [Releases](https://github.com/Justsenger/ExHyperV-USBProxy/releases) 页面下载最新的 `USBProxy.exe`。
