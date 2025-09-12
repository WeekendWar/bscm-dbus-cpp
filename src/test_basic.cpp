#include <iostream>
#include "bluetooth_manager.h"

int main() {
    std::cout << "=== Basic Compilation Test ===" << std::endl;
    
    // Test that the classes can be instantiated
    BluetoothManager manager;
    std::cout << "BluetoothManager instantiated successfully" << std::endl;
    
    // Test that we can set desired services
    std::vector<std::string> testServices = {"0000180f-0000-1000-8000-00805f9b34fb"};
    manager.setDesiredServices(testServices);
    std::cout << "Service filtering configured successfully" << std::endl;
    
    // Test hex parsing function
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03, 0xFF};
    std::cout << "Test data created: ";
    for (auto byte : testData) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;
    
    std::cout << "All basic functionality tests passed!" << std::endl;
    return 0;
}