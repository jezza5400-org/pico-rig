# pico-rig

A Raspberry Pi Pico-powered USB radio interface that integrates a high-performance USB sound card and a virtual serial port into a single device. It features low-latency hardware PTT via the USB serial RTS line, routing all programming data, control signals, and audio through a unified connection.

## Project Overview & Architecture

**pico-rig** is a next-generation radio interface powered by the **Raspberry Pi Pico 2** and the advanced **RP2354A** microcontroller. It packs a high-performance USB sound card and a virtual serial interface into a single, compact hardware footprint.

Unlike legacy multi-cable setups, this design exploits the processing power and flexible I/O of the RP2354A to unify all audio signals, hardware PTT switching, and radio programming lines into a single physical cable harness tailored for hand-held transceivers (such as K-type Kenwood/Baofeng style dual-pin configurations).

### Hardware-Driven PTT Logic

* **Direct Control:** This project bypasses network middleware layers entirely. By adding `PTT /dev/ttyACM0 RTS` to your configuration file, **Direwolf** gains direct, low-level custody of the virtual serial device node.
* **Out-of-Band Switching:** Transmission switching relies entirely on standard serial **RTS (Request to Send)** modem control signals. These line states exist completely out-of-band, requiring absolutely zero software serial text parsing or complex ASCII command handling inside the Pico 2 firmware.
* **Zero-Latency Triggers:** When Direwolf prepares a packet, it toggles the USB CDC line state directly on the host driver interface. The RP2354A instantly intercepts this hardware event and drives a physical GPIO pin to key the transceiver in perfect sync with the audio buffers.
* **No Intermittent VOX:** By choosing explicit hardware-keyed PTT over audio-triggered VOX, the station avoids common failure modes like truncated preambles (clipping packet headers), extended carrier hang-time, and accidental hot-mic transmissions caused by computer system alert sounds.

### Data & Programming Isolation

* **Simultaneous Operations:** The Pico 2 firmware presents a standard USB CDC ACM serial profile to the operating system.
* **CHIRP Pass-Through:** While the RTS line is reserved exclusively for Direwolf's fast PTT switching, the underlying hardware RX/TX data lines remain unconstrained. This allows you to pass radio programming data natively down the exact same cable interface using software like **CHIRP** without having to physically reconfigure hardware jumpers or disconnect the unit.

## Development & Build Configuration

The development workspace for this repository relies on a modern embedded environment configured for the RP2350/RP2354 series with the following toolchain parameters:

| Component | Technology | Description |
| :--- | :--- | :--- |
| **Target Hardware** | [RP2354A / Pico 2](https://www.raspberrypi.com/documentation/microcontrollers/microcontroller-chips.html#rp2350) | High-performance ARM Cortex-M33 architecture |
| **Build Configuration** | [CMake](https://cmake.org) | Handles cross-platform project structure and asset definitions |
| **Build Execution** | [Ninja](https://ninja-build.org) | Provides lightning-fast, parallel compilation execution |
| **Code Compilation** | [Arm GNU Toolchain](https://developer.arm.com/tools-and-software/gnu-toolchain) | Target compiler cross-assembling code for `arm-none-eabi` |
| **Completions & Diagnostics** | [Clangd](https://clangd.llvm.org/) | Delivers precise, AST-backed code intelligence and linting |
| **Code Formatting** | [Clang-Format](https://clang.llvm.org/docs/ClangFormat.html) | Enforces unified, clean style guidelines across source files |

The Raspberry Pi Pico C/C++ SDK install instructions can be found here: [pico-setup](https://github.com/raspberrypi/pico-setup/blob/master/README.md#setup-sdk--picotool)
