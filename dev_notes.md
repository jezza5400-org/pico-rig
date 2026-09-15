# Dev-Notes

A quick reference guide for building, flashing, and debugging Raspberry Pi Pico projects from the terminal using OpenOCD and `arm-none-eabi-gdb`.

## Building the Project

This project uses `CMakePresets.json` to automatically manage build directories, toolchains, and optimization profiles.

### 1. Configure

Run the configure command for your desired target build type. CMake will automatically create the required build directory:

```bash
# Debug Mode (Debug symbols enabled, optimizations off)
cmake --preset debug

# Release Mode (Maximum speed optimization -O3)
cmake --preset release

# MinSize Mode (Optimized for smallest flash size -Os)
cmake --preset minsize
```

### 2. Compile

Once configured, run the compile step:
Bash

```bash
# Compile Debug build
cmake --build --preset debug

# Compile Max Speed Release build
cmake --build --preset release

# Compile Smallest Size Release build
cmake --build --preset minsize
```

## 2. Flashing & Uploading Firmware

Using picotool to flash directly:

| Action | Command & Flags | When to Use / Prerequisite |
| :--- | :--- | :--- |
| **Inspect Board/File** | `picotool info -a` | Check details, memory allocation, and pins. |
| **Flash & Run File** | `picotool load <file.uf2> -x` | `-x` reboots the board to execute the code immediately. |
| **Hands-Free Flash** | `picotool load -f <file.uf2> -x` | `-f` forces a running Pico with USB serial into BOOTSEL mode. |
| **Verify Memory** | `picotool verify <file.uf2>` | Compares device memory against a local file. |
| **Backup Memory** | `picotool save -a <output.uf2>` | `-a` saves the entire active flash area to your computer. |
| **Force Reboot** | `picotool reboot` | Safely reboots the board into normal runtime execution. |

Using OpenOCD via CMSIS-DAP Debug Probe to flash the `.elf` binary to the rp2350.

```bash
sudo ~/.pico-sdk/openocd/0.12.0+dev/openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program build/your_project.elf verify reset exit"
```

## 3. Terminal Debugging Setup

Debugging requires two terminal windows running side by side.

### Terminal 1: OpenOCD Server

Keep this running in the background to handle the hardware connection:

```bash
sudo ~/.pico-sdk/openocd/0.12.0+dev/openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000"
```

### Terminal 2: Connect GDB

Launch the ARM GDB debugger pointing to the compiled `.elf` file:

```bash
arm-none-eabi-gdb build/your_project.elf
```

## 4. Essential GDB Commands

Once inside the GDB prompt `(gdb)`:

### Target Connection & Control

| Command | Short | Description |
| --- | --- | --- |
| `target extended-remote localhost:3333` | `tar ext :3333` | Connect GDB to the running OpenOCD server |
| `monitor reset halt` | `mon res halt` | Reset the MCU and freeze execution instantly |
| `monitor reset init` | `mon res init` | Reset the MCU and freeze execution after init |
| `monitor targets` | | Displays the status of Core 0 (rp2350.cm0) and Core 1 (rp2350.cm1) |
| `load` | | Flash the loaded `.elf` file directly over SWD |
| `continue` | `c` | Resume program execution |
| `quit` | `q` | Exit GDB |

### Breakpoints

| Command | Short | Description |
| --- | --- | --- |
| `break main` | `b main` | Set a breakpoint at the entry of `main()` |
| `break main.cpp:25` | `b main.cpp:25` | Set a breakpoint at a specific file and line number |
| `info breakpoints` | `info b` | List all active breakpoints and their IDs |
| `delete 1` | `d 1` | Delete breakpoint #1 |
| `disable 1` | | Temporarily disable breakpoint #1 |
| `enable 1` | | Re-enable breakpoint #1 |

### Stepping Code

| Command | Short | Description |
| --- | --- | --- |
| `next` | `n` | **Step Over:** Execute current line (does not jump into functions) |
| `step` | `s` | **Step Into:** Execute current line (jumps inside function calls) |
| `finish` | `fin` | **Step Out:** Run until current function returns, then pause |
| `advance` | `adv` | **Advance until:** Advance until specified line number |

### Inspecting State

| Command | Short | Description |
| --- | --- | --- |
| `print my_var` | `p my_var` | Print the current value of a variable |
| `print/x my_var` | `p/x my_var` | Print value in Hexadecimal format |
| `display my_var` | | Automatically print `my_var` every time execution stops |
| `backtrace` | `bt` | Show the execution stack frame call history |
| `info locals` | | Show all local variables in current scope |
| `info registers` | | Display current CPU registers |

## .gdbinit file

The .gdbinit file automatically runs GDB commands on startup so you don't have to type them manually every session.

* `target extended-remote localhost:3333`: Connects GDB to OpenOCD's debug server on port 3333.
* `monitor reset init`: Resets and initializes the microcontroller for debugging via OpenOCD.
* `load`: Flashes the binary file onto the microcontroller.
* `skip file **/tinyusb/**`: Prevents stepping into TinyUSB library code while debugging.
* `#layout regs`: *(Disabled)* Toggles GDB layout to display CPU registers.
* `#tbreak main`: *(Disabled)* Sets a temporary breakpoint at `main()`.
* `continue`: Resumes target execution until a breakpoint or stop signal.
