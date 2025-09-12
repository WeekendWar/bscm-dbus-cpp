#include "bluetooth_manager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdint>

void printDeviceInfo(const BluetoothDevice& device, int index) {
    std::cout << "[" << index << "] " << device.name;
    if (device.name.empty()) {
        std::cout << "Unknown Device";
    }
    std::cout << " (" << device.address << ")";
    if (device.connected) {
        std::cout << " [CONNECTED]";
    }
    std::cout << std::endl;
    
    if (!device.services.empty()) {
        std::cout << "    Services: ";
        for (size_t i = 0; i < device.services.size() && i < 3; i++) {
            std::cout << device.services[i];
            if (i < device.services.size() - 1 && i < 2) std::cout << ", ";
        }
        if (device.services.size() > 3) {
            std::cout << " (+" << (device.services.size() - 3) << " more)";
        }
        std::cout << std::endl;
    }
}

void printCharacteristicInfo(const BluetoothCharacteristic& characteristic, int index) {
    std::cout << "[" << index << "] UUID: " << characteristic.uuid << std::endl;
    std::cout << "    Path: " << characteristic.path << std::endl;
    std::cout << "    Flags: ";
    for (const auto& flag : characteristic.flags) {
        std::cout << flag << " ";
    }
    std::cout << std::endl;
}

void printHexData(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        if (i < data.size() - 1) std::cout << " ";
    }
    std::cout << std::dec << std::endl;
}

std::vector<uint8_t> parseHexString(const std::string& hexStr) {
    std::vector<uint8_t> data;
    std::string cleanStr = hexStr;
    
    // Remove spaces and 0x prefixes
    std::string result;
    for (size_t i = 0; i < cleanStr.length(); i++) {
        if (cleanStr[i] != ' ' && cleanStr[i] != '\t') {
            if (i < cleanStr.length() - 1 && cleanStr[i] == '0' && cleanStr[i+1] == 'x') {
                i++; // Skip the 'x'
                continue;
            }
            result += cleanStr[i];
        }
    }
    
    for (size_t i = 0; i < result.length(); i += 2) {
        if (i + 1 < result.length()) {
            std::string byteStr = result.substr(i, 2);
            try {
                uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
                data.push_back(byte);
            } catch (const std::exception& e) {
                std::cerr << "Invalid hex string: " << byteStr << std::endl;
                break;
            }
        }
    }
    
    return data;
}

int getUserChoice(int maxChoice) {
    int choice;
    std::cout << "Enter your choice (0-" << maxChoice << "): ";
    std::cin >> choice;
    
    if (std::cin.fail() || choice < 0 || choice > maxChoice) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return -1;
    }
    
    return choice;
}

int main() {
    std::cout << "=== Bluetooth Device Manager ===" << std::endl;
    std::cout << "C++ app using BlueZ over D-Bus" << std::endl << std::endl;
    
    BluetoothManager manager;
    
    if (!manager.initialize()) {
        std::cerr << "Failed to initialize Bluetooth manager" << std::endl;
        return 1;
    }
    
    // Set up notification callback
    manager.setNotificationCallback([](const std::string& charPath, const std::vector<uint8_t>& data) {
        std::cout << "\n*** NOTIFICATION from " << charPath << " ***" << std::endl;
        std::cout << "Data: ";
        printHexData(data);
        std::cout << "ASCII: ";
        for (uint8_t byte : data) {
            if (byte >= 32 && byte <= 126) {
                std::cout << static_cast<char>(byte);
            } else {
                std::cout << ".";
            }
        }
        std::cout << std::endl << std::endl;
    });
    
    while (true) {
        std::cout << "\n=== Main Menu ===" << std::endl;
        std::cout << "1. Scan for devices" << std::endl;
        std::cout << "2. Set service filter" << std::endl;
        std::cout << "3. List devices with desired services" << std::endl;
        std::cout << "4. Connect to device" << std::endl;
        std::cout << "5. Disconnect from device" << std::endl;
        std::cout << "6. Manage characteristics" << std::endl;
        std::cout << "7. Process notifications" << std::endl;
        std::cout << "0. Exit" << std::endl;
        
        int choice = getUserChoice(7);
        
        switch (choice) {
            case 1: {
                std::cout << "\nStarting device scan..." << std::endl;
                manager.startDiscovery();
                manager.scanForDevices(10);
                manager.stopDiscovery();
                
                auto devices = manager.getAllDevices();
                std::cout << "\nFound " << devices.size() << " devices:" << std::endl;
                for (size_t i = 0; i < devices.size(); i++) {
                    printDeviceInfo(devices[i], i);
                }
                break;
            }
            
            case 2: {
                std::cout << "\nEnter desired service UUIDs (comma-separated, or 'none' to clear): ";
                std::cin.ignore();
                std::string input;
                std::getline(std::cin, input);
                
                if (input == "none") {
                    manager.setDesiredServices({});
                } else {
                    std::vector<std::string> services;
                    std::stringstream ss(input);
                    std::string service;
                    
                    while (std::getline(ss, service, ',')) {
                        // Trim whitespace
                        service.erase(0, service.find_first_not_of(" \t"));
                        service.erase(service.find_last_not_of(" \t") + 1);
                        if (!service.empty()) {
                            services.push_back(service);
                        }
                    }
                    
                    manager.setDesiredServices(services);
                }
                break;
            }
            
            case 3: {
                auto devices = manager.getDevicesWithDesiredServices();
                std::cout << "\nDevices with desired services:" << std::endl;
                if (devices.empty()) {
                    std::cout << "No devices found with desired services." << std::endl;
                } else {
                    for (size_t i = 0; i < devices.size(); i++) {
                        printDeviceInfo(devices[i], i);
                    }
                }
                break;
            }
            
            case 4: {
                auto devices = manager.getAllDevices();
                if (devices.empty()) {
                    std::cout << "No devices found. Please scan first." << std::endl;
                    break;
                }
                
                std::cout << "\nSelect device to connect:" << std::endl;
                for (size_t i = 0; i < devices.size(); i++) {
                    printDeviceInfo(devices[i], i);
                }
                
                int deviceChoice = getUserChoice(devices.size() - 1);
                if (deviceChoice >= 0) {
                    manager.connectToDevice(devices[deviceChoice].path);
                }
                break;
            }
            
            case 5: {
                auto devices = manager.getAllDevices();
                std::vector<BluetoothDevice> connectedDevices;
                for (const auto& device : devices) {
                    if (device.connected) {
                        connectedDevices.push_back(device);
                    }
                }
                
                if (connectedDevices.empty()) {
                    std::cout << "No connected devices." << std::endl;
                    break;
                }
                
                std::cout << "\nSelect device to disconnect:" << std::endl;
                for (size_t i = 0; i < connectedDevices.size(); i++) {
                    printDeviceInfo(connectedDevices[i], i);
                }
                
                int deviceChoice = getUserChoice(connectedDevices.size() - 1);
                if (deviceChoice >= 0) {
                    manager.disconnectFromDevice(connectedDevices[deviceChoice].path);
                }
                break;
            }
            
            case 6: {
                auto devices = manager.getAllDevices();
                std::vector<BluetoothDevice> connectedDevices;
                for (const auto& device : devices) {
                    if (device.connected) {
                        connectedDevices.push_back(device);
                    }
                }
                
                if (connectedDevices.empty()) {
                    std::cout << "No connected devices. Please connect to a device first." << std::endl;
                    break;
                }
                
                std::cout << "\nSelect connected device:" << std::endl;
                for (size_t i = 0; i < connectedDevices.size(); i++) {
                    printDeviceInfo(connectedDevices[i], i);
                }
                
                int deviceChoice = getUserChoice(connectedDevices.size() - 1);
                if (deviceChoice < 0) break;
                
                auto characteristics = manager.getCharacteristics(connectedDevices[deviceChoice].path);
                if (characteristics.empty()) {
                    std::cout << "No characteristics found for this device." << std::endl;
                    break;
                }
                
                while (true) {
                    std::cout << "\n=== Characteristic Management ===" << std::endl;
                    std::cout << "Device: " << connectedDevices[deviceChoice].name << std::endl;
                    std::cout << "\nCharacteristics:" << std::endl;
                    for (size_t i = 0; i < characteristics.size(); i++) {
                        printCharacteristicInfo(characteristics[i], i);
                    }
                    
                    std::cout << "\nActions:" << std::endl;
                    std::cout << "1. Enable notifications" << std::endl;
                    std::cout << "2. Disable notifications" << std::endl;
                    std::cout << "3. Read characteristic" << std::endl;
                    std::cout << "4. Write to characteristic" << std::endl;
                    std::cout << "0. Back to main menu" << std::endl;
                    
                    int action = getUserChoice(4);
                    if (action == 0) break;
                    
                    std::cout << "Select characteristic: ";
                    int charChoice = getUserChoice(characteristics.size() - 1);
                    if (charChoice < 0) continue;
                    
                    const auto& selectedChar = characteristics[charChoice];
                    
                    switch (action) {
                        case 1:
                            manager.enableNotifications(selectedChar.path);
                            break;
                            
                        case 2:
                            manager.disableNotifications(selectedChar.path);
                            break;
                            
                        case 3: {
                            auto data = manager.readCharacteristic(selectedChar.path);
                            std::cout << "Read data: ";
                            printHexData(data);
                            break;
                        }
                        
                        case 4: {
                            std::cout << "Enter hex data to write (e.g., '01 02 03' or '010203'): ";
                            std::cin.ignore();
                            std::string hexInput;
                            std::getline(std::cin, hexInput);
                            
                            auto data = parseHexString(hexInput);
                            if (!data.empty()) {
                                manager.writeCharacteristic(selectedChar.path, data);
                            } else {
                                std::cout << "Invalid hex data" << std::endl;
                            }
                            break;
                        }
                    }
                }
                break;
            }
            
            case 7: {
                std::cout << "\nProcessing notifications for 10 seconds..." << std::endl;
                std::cout << "Press Ctrl+C to stop early." << std::endl;
                
                auto startTime = std::chrono::steady_clock::now();
                auto endTime = startTime + std::chrono::seconds(10);
                
                while (std::chrono::steady_clock::now() < endTime) {
                    manager.processNotifications();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                std::cout << "Finished processing notifications." << std::endl;
                break;
            }
            
            case 0:
                std::cout << "Exiting..." << std::endl;
                return 0;
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
        }
    }
    
    return 0;
}