#include "bluetooth_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>

BluetoothManager::BluetoothManager() {
}

BluetoothManager::~BluetoothManager() {
    stopDiscovery();
}

bool BluetoothManager::initialize() {
    if (!dbus.connect()) {
        std::cerr << "Failed to connect to D-Bus" << std::endl;
        return false;
    }
    
    if (!findAdapter()) {
        std::cerr << "No Bluetooth adapter found" << std::endl;
        return false;
    }
    
    // Add signal match for property changes and interface additions
    dbus.addSignalMatch("type='signal',sender='org.bluez'");
    
    std::cout << "Bluetooth manager initialized with adapter: " << adapterPath << std::endl;
    return true;
}

bool BluetoothManager::findAdapter() {
    DBusMessage* reply = dbus.callMethod("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    
    if (!reply) {
        return false;
    }
    
    DBusMessageIter iter, dict_iter;
    if (!dbus_message_iter_init(reply, &iter) || 
        dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        dbus_message_unref(reply);
        return false;
    }
    
    dbus_message_iter_recurse(&iter, &dict_iter);
    
    while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry_iter, interfaces_iter;
        const char* path;
        
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &path);
        dbus_message_iter_next(&entry_iter);
        
        if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&entry_iter, &interfaces_iter);
            
            while (dbus_message_iter_get_arg_type(&interfaces_iter) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter iface_entry_iter;
                const char* interface;
                
                dbus_message_iter_recurse(&interfaces_iter, &iface_entry_iter);
                dbus_message_iter_get_basic(&iface_entry_iter, &interface);
                
                if (std::string(interface) == "org.bluez.Adapter1") {
                    adapterPath = std::string(path);
                    dbus_message_unref(reply);
                    return true;
                }
                
                dbus_message_iter_next(&interfaces_iter);
            }
        }
        
        dbus_message_iter_next(&dict_iter);
    }
    
    dbus_message_unref(reply);
    return false;
}

bool BluetoothManager::startDiscovery() {
    if (adapterPath.empty()) return false;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", adapterPath, "org.bluez.Adapter1", "StartDiscovery");
    
    if (reply) {
        dbus_message_unref(reply);
        std::cout << "Started Bluetooth discovery" << std::endl;
        return true;
    }
    
    return false;
}

bool BluetoothManager::stopDiscovery() {
    if (adapterPath.empty()) return false;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", adapterPath, "org.bluez.Adapter1", "StopDiscovery");
    
    if (reply) {
        dbus_message_unref(reply);
        std::cout << "Stopped Bluetooth discovery" << std::endl;
        return true;
    }
    
    return false;
}

void BluetoothManager::scanForDevices(int timeoutSeconds) {
    std::cout << "Scanning for devices for " << timeoutSeconds << " seconds..." << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(timeoutSeconds);
    
    while (std::chrono::steady_clock::now() < endTime) {
        dbus.processMessages(1000);
        discoverDevices();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "Scan complete. Found " << devices.size() << " devices." << std::endl;
}

void BluetoothManager::discoverDevices() {
    DBusMessage* reply = dbus.callMethod("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    
    if (!reply) return;
    
    DBusMessageIter iter, dict_iter;
    if (!dbus_message_iter_init(reply, &iter) || 
        dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        dbus_message_unref(reply);
        return;
    }
    
    dbus_message_iter_recurse(&iter, &dict_iter);
    
    while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry_iter, interfaces_iter;
        const char* path;
        
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &path);
        dbus_message_iter_next(&entry_iter);
        
        std::string pathStr(path);
        if (pathStr.find("/org/bluez/hci") != std::string::npos && 
            pathStr.find("/dev_") != std::string::npos) {
            
            if (devices.find(pathStr) == devices.end()) {
                BluetoothDevice device;
                device.path = pathStr;
                parseDeviceProperties(pathStr, device);
                devices[pathStr] = device;
            }
        }
        
        dbus_message_iter_next(&dict_iter);
    }
    
    dbus_message_unref(reply);
}

void BluetoothManager::parseDeviceProperties(const std::string& devicePath, BluetoothDevice& device) {
    device.address = dbus.getStringProperty("org.bluez", devicePath, "org.bluez.Device1", "Address");
    device.name = dbus.getStringProperty("org.bluez", devicePath, "org.bluez.Device1", "Name");
    device.connected = dbus.getBoolProperty("org.bluez", devicePath, "org.bluez.Device1", "Connected");
    
    // Get UUIDs (services)
    DBusMessage* reply = dbus.callMethodWithArgs("org.bluez", devicePath, "org.freedesktop.DBus.Properties", "Get",
        [](DBusMessage* msg) {
            const char* iface = "org.bluez.Device1";
            const char* prop = "UUIDs";
            dbus_message_append_args(msg, 
                DBUS_TYPE_STRING, &iface,
                DBUS_TYPE_STRING, &prop,
                DBUS_TYPE_INVALID);
        });
    
    if (reply) {
        DBusMessageIter iter, variant_iter, array_iter;
        if (dbus_message_iter_init(reply, &iter) && 
            dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
            dbus_message_iter_recurse(&iter, &variant_iter);
            if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse(&variant_iter, &array_iter);
                
                device.services.clear();
                while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING) {
                    const char* uuid;
                    dbus_message_iter_get_basic(&array_iter, &uuid);
                    device.services.push_back(std::string(uuid));
                    dbus_message_iter_next(&array_iter);
                }
            }
        }
        dbus_message_unref(reply);
    }
}

void BluetoothManager::setDesiredServices(const std::vector<std::string>& services) {
    desiredServices = services;
    std::cout << "Set desired services: ";
    for (const auto& service : services) {
        std::cout << service << " ";
    }
    std::cout << std::endl;
}

std::vector<BluetoothDevice> BluetoothManager::getDevicesWithDesiredServices() {
    std::vector<BluetoothDevice> filteredDevices;
    
    for (const auto& devicePair : devices) {
        if (hasDesiredService(devicePair.second)) {
            filteredDevices.push_back(devicePair.second);
        }
    }
    
    return filteredDevices;
}

bool BluetoothManager::hasDesiredService(const BluetoothDevice& device) {
    if (desiredServices.empty()) {
        return true; // If no filter set, include all devices
    }
    
    for (const auto& desiredService : desiredServices) {
        for (const auto& deviceService : device.services) {
            if (deviceService.find(desiredService) != std::string::npos) {
                return true;
            }
        }
    }
    
    return false;
}

bool BluetoothManager::connectToDevice(const std::string& devicePath) {
    std::cout << "Connecting to device: " << devicePath << std::endl;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", devicePath, "org.bluez.Device1", "Connect");
    
    if (reply) {
        dbus_message_unref(reply);
        
        // Wait a bit for connection to establish
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            bool connected = dbus.getBoolProperty("org.bluez", devicePath, "org.bluez.Device1", "Connected");
            if (connected) {
                std::cout << "Successfully connected to device" << std::endl;
                devices[devicePath].connected = true;
                return true;
            }
        }
    }
    
    std::cerr << "Failed to connect to device" << std::endl;
    return false;
}

bool BluetoothManager::disconnectFromDevice(const std::string& devicePath) {
    std::cout << "Disconnecting from device: " << devicePath << std::endl;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", devicePath, "org.bluez.Device1", "Disconnect");
    
    if (reply) {
        dbus_message_unref(reply);
        devices[devicePath].connected = false;
        std::cout << "Disconnected from device" << std::endl;
        return true;
    }
    
    return false;
}

std::vector<BluetoothCharacteristic> BluetoothManager::getCharacteristics(const std::string& devicePath) {
    std::vector<BluetoothCharacteristic> characteristics;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    
    if (!reply) return characteristics;
    
    DBusMessageIter iter, dict_iter;
    if (!dbus_message_iter_init(reply, &iter) || 
        dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
        dbus_message_unref(reply);
        return characteristics;
    }
    
    dbus_message_iter_recurse(&iter, &dict_iter);
    
    while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry_iter, interfaces_iter;
        const char* path;
        
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &path);
        dbus_message_iter_next(&entry_iter);
        
        std::string pathStr(path);
        if (pathStr.find(devicePath) == 0 && pathStr.find("/char") != std::string::npos) {
            
            if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse(&entry_iter, &interfaces_iter);
                
                while (dbus_message_iter_get_arg_type(&interfaces_iter) == DBUS_TYPE_DICT_ENTRY) {
                    DBusMessageIter iface_entry_iter;
                    const char* interface;
                    
                    dbus_message_iter_recurse(&interfaces_iter, &iface_entry_iter);
                    dbus_message_iter_get_basic(&iface_entry_iter, &interface);
                    
                    if (std::string(interface) == "org.bluez.GattCharacteristic1") {
                        BluetoothCharacteristic characteristic;
                        characteristic.path = pathStr;
                        parseCharacteristicProperties(pathStr, characteristic);
                        characteristics.push_back(characteristic);
                        break;
                    }
                    
                    dbus_message_iter_next(&interfaces_iter);
                }
            }
        }
        
        dbus_message_iter_next(&dict_iter);
    }
    
    dbus_message_unref(reply);
    return characteristics;
}

void BluetoothManager::parseCharacteristicProperties(const std::string& charPath, BluetoothCharacteristic& characteristic) {
    characteristic.uuid = dbus.getStringProperty("org.bluez", charPath, "org.bluez.GattCharacteristic1", "UUID");
    characteristic.service_path = dbus.getStringProperty("org.bluez", charPath, "org.bluez.GattCharacteristic1", "Service");
    
    // Get flags
    DBusMessage* reply = dbus.callMethodWithArgs("org.bluez", charPath, "org.freedesktop.DBus.Properties", "Get",
        [](DBusMessage* msg) {
            const char* iface = "org.bluez.GattCharacteristic1";
            const char* prop = "Flags";
            dbus_message_append_args(msg, 
                DBUS_TYPE_STRING, &iface,
                DBUS_TYPE_STRING, &prop,
                DBUS_TYPE_INVALID);
        });
    
    if (reply) {
        DBusMessageIter iter, variant_iter, array_iter;
        if (dbus_message_iter_init(reply, &iter) && 
            dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
            dbus_message_iter_recurse(&iter, &variant_iter);
            if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse(&variant_iter, &array_iter);
                
                characteristic.flags.clear();
                while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING) {
                    const char* flag;
                    dbus_message_iter_get_basic(&array_iter, &flag);
                    characteristic.flags.push_back(std::string(flag));
                    dbus_message_iter_next(&array_iter);
                }
            }
        }
        dbus_message_unref(reply);
    }
}

bool BluetoothManager::enableNotifications(const std::string& characteristicPath) {
    std::cout << "Enabling notifications for: " << characteristicPath << std::endl;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", characteristicPath, "org.bluez.GattCharacteristic1", "StartNotify");
    
    if (reply) {
        dbus_message_unref(reply);
        notifyingCharacteristics.insert(characteristicPath);
        std::cout << "Notifications enabled" << std::endl;
        return true;
    }
    
    std::cerr << "Failed to enable notifications" << std::endl;
    return false;
}

bool BluetoothManager::disableNotifications(const std::string& characteristicPath) {
    std::cout << "Disabling notifications for: " << characteristicPath << std::endl;
    
    DBusMessage* reply = dbus.callMethod("org.bluez", characteristicPath, "org.bluez.GattCharacteristic1", "StopNotify");
    
    if (reply) {
        dbus_message_unref(reply);
        notifyingCharacteristics.erase(characteristicPath);
        std::cout << "Notifications disabled" << std::endl;
        return true;
    }
    
    return false;
}

bool BluetoothManager::writeCharacteristic(const std::string& characteristicPath, const std::vector<uint8_t>& data) {
    std::cout << "Writing to characteristic: " << characteristicPath << std::endl;
    
    DBusMessage* reply = dbus.callMethodWithArgs("org.bluez", characteristicPath, "org.bluez.GattCharacteristic1", "WriteValue",
        [&](DBusMessage* msg) {
            DBusMessageIter iter, array_iter, options_iter;
            dbus_message_iter_init_append(msg, &iter);
            
            // Append byte array
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "y", &array_iter);
            for (uint8_t byte : data) {
                dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_BYTE, &byte);
            }
            dbus_message_iter_close_container(&iter, &array_iter);
            
            // Append empty options dict
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &options_iter);
            dbus_message_iter_close_container(&iter, &options_iter);
        });
    
    if (reply) {
        dbus_message_unref(reply);
        std::cout << "Write successful" << std::endl;
        return true;
    }
    
    std::cerr << "Write failed" << std::endl;
    return false;
}

std::vector<uint8_t> BluetoothManager::readCharacteristic(const std::string& characteristicPath) {
    std::vector<uint8_t> data;
    
    DBusMessage* reply = dbus.callMethodWithArgs("org.bluez", characteristicPath, "org.bluez.GattCharacteristic1", "ReadValue",
        [](DBusMessage* msg) {
            DBusMessageIter iter, options_iter;
            dbus_message_iter_init_append(msg, &iter);
            
            // Append empty options dict
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &options_iter);
            dbus_message_iter_close_container(&iter, &options_iter);
        });
    
    if (reply) {
        DBusMessageIter iter, array_iter;
        if (dbus_message_iter_init(reply, &iter) && 
            dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&iter, &array_iter);
            
            while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_BYTE) {
                uint8_t byte;
                dbus_message_iter_get_basic(&array_iter, &byte);
                data.push_back(byte);
                dbus_message_iter_next(&array_iter);
            }
        }
        dbus_message_unref(reply);
    }
    
    return data;
}

void BluetoothManager::processNotifications() {
    dbus.processMessages(100);
}

void BluetoothManager::setNotificationCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
    notificationCallback = callback;
}

std::vector<BluetoothDevice> BluetoothManager::getAllDevices() {
    std::vector<BluetoothDevice> deviceList;
    for (const auto& devicePair : devices) {
        deviceList.push_back(devicePair.second);
    }
    return deviceList;
}

void BluetoothManager::updateDeviceInfo() {
    for (auto& devicePair : devices) {
        parseDeviceProperties(devicePair.first, devicePair.second);
    }
}