# ExHyperV-USBProxy

[![License](https://img.shields.io/badge/license-GPL--v3-red.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-0078D4.svg)]()
[![Project](https://img.shields.io/badge/Part%20of-ExHyperV-8957e5.svg)](https://github.com/Justsenger/ExHyperV)

`ExHyperV-USBProxy` is a core dependency of the **[ExHyperV](https://github.com/Justsenger/ExHyperV)** project, designed to deliver high-performance USBIP transport inside a Hyper-V environment.

## 📖 Overview

When using USB devices inside a Hyper-V virtual machine, traditional network-based solutions are often limited by virtual NIC performance or complex configuration requirements. This project acts as a first-stage proxy, tunneling standard **TCP/IP (USBIP)** traffic over **AF_HYPERV (Hyper-V Sockets)** instead.

### Why AF_HYPERV?

- **High Performance**: Bypasses the traditional network stack, eliminating encapsulation overhead.
- **Low Latency**: Host and Guest communicate directly over VMBus.
- **Zero Configuration**: No IP address or NAT setup required — the connection just works.

## Features

- **Protocol Bridging**: Seamlessly forwards USBIP TCP streams over AF_HYPERV.
- **Optimized Forwarding**: Data relay logic tuned specifically for the Hyper-V environment.
- **Lightweight**: Written in C++ with no unnecessary dependencies and minimal runtime overhead.
- **Automated Mounting**: Integrates with ExHyperV to support fully automated USB attach workflows.

## Requirements

- Windows 10/11 or Windows Server 2016+

## Installation

Download the latest `USBProxy.exe` from the [Releases](https://github.com/Justsenger/ExHyperV-USBProxy/releases) page.
