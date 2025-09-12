cmake_minimum_required(VERSION 3.10)
project(bscm-dbus-cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Include directories
include_directories(${DBUS_INCLUDE_DIRS})

# Add executable
add_executable(bscm-bluetooth-manager
    src/main.cpp
    src/bluetooth_manager.cpp
    src/dbus_helper.cpp
)

# Link libraries
target_link_libraries(bscm-bluetooth-manager ${DBUS_LIBRARIES})

# Compiler flags
target_compile_options(bscm-bluetooth-manager PRIVATE ${DBUS_CFLAGS_OTHER})

# Add test executable
add_executable(test-basic
    src/test_basic.cpp
    src/bluetooth_manager.cpp
    src/dbus_helper.cpp
)

# Link libraries for test
target_link_libraries(test-basic ${DBUS_LIBRARIES})
target_compile_options(test-basic PRIVATE ${DBUS_CFLAGS_OTHER})