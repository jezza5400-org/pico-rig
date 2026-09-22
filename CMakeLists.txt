# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
	set(USERHOME $ENV{USERPROFILE})
else()
	set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
	include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")
#set(PICO_BOARD pimoroni_pico_plus2_w_rp2350 CACHE STRING "Board type")

# Use latest TinyUSB
set(PICO_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/libs/tinyusb CACHE PATH "Path to custom TinyUSB")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-rig C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Automatically find all .cpp files inside the src directory tree
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# Add executable. Default name is the project name, version 0.1
add_executable(pico-rig ${SRC_FILES} )

# Add target definitions to overwrite the SDK defaults explicitly
target_compile_definitions(pico-rig PRIVATE
	CFG_TUD_ENABLED=1
	CFG_TUD_CDC=1
	CFG_TUD_AUDIO=1
)

pico_set_program_name(pico-rig "pico-rig")
pico_set_program_version(pico-rig "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico-rig 0)
pico_enable_stdio_usb(pico-rig 1)

# Add the standard library to the build
target_link_libraries(pico-rig
	pico_stdlib
	hardware_psram
	tinyusb_device
	tinyusb_additions
)

# Add the standard include files to the build
target_include_directories(pico-rig PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/include
)

pico_add_extra_outputs(pico-rig)
