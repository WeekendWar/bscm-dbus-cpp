#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "dbus_helper.h"

struct BluetoothDevice
{
  std::string              path;
  std::string              address;
  std::string              name;
  std::vector<std::string> services;
  bool                     connected = false;
};

struct BluetoothCharacteristic
{
  std::string              path;
  std::string              uuid;
  std::vector<std::string> flags;
  std::string              service_path;
};

class BluetoothManager
{
public:
  BluetoothManager();
  ~BluetoothManager();

  bool initialize();

  // Device scanning
  bool startDiscovery();
  bool stopDiscovery();
  void scanForDevices(int timeoutSeconds = 10);

  // Service filtering
  void setDesiredServices(const std::vector<std::string>& services);
  std::vector<BluetoothDevice> getDevicesWithDesiredServices();

  // Device connection
  bool connectToDevice(const std::string& devicePath);
  bool disconnectFromDevice(const std::string& devicePath);

  // Service and characteristic discovery
  std::vector<BluetoothCharacteristic> getCharacteristics(
    const std::string& devicePath);

  // Characteristic operations
  bool enableNotifications(const std::string& characteristicPath);
  bool disableNotifications(const std::string& characteristicPath);
  bool writeCharacteristic(const std::string&          characteristicPath,
                           const std::vector<uint8_t>& data);
  std::vector<uint8_t> readCharacteristic(
    const std::string& characteristicPath);

  // Notification handling
  void processNotifications();
  void setNotificationCallback(
    std::function<void(const std::string&, const std::vector<uint8_t>&)>
      callback);

  // Device management
  std::vector<BluetoothDevice> getAllDevices();
  void                         updateDeviceInfo();

private:
  DBusHelper                             dbus_;
  std::vector<std::string>               desiredServices_;
  std::map<std::string, BluetoothDevice> devices_;
  std::set<std::string>                  notifyingCharacteristics_;
  std::function<void(const std::string&, const std::vector<uint8_t>&)>
    notificationCallback_;

  std::string adapterPath_;

  bool findAdapter();
  void discoverDevices();
  void parseDeviceProperties(const std::string& devicePath,
                             BluetoothDevice&   device);
  void parseCharacteristicProperties(const std::string&       charPath,
                                     BluetoothCharacteristic& characteristic);
  bool hasDesiredService(const BluetoothDevice& device);
  static DBusHandlerResult messageFilter(DBusConnection* connection,
                                         DBusMessage*    message,
                                         void*           userData);
};

#endif  // BLUETOOTH_MANAGER_H