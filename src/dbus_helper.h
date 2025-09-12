#ifndef DBUS_HELPER_H
#define DBUS_HELPER_H

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

class DBusHelper {
public:
    DBusHelper();
    ~DBusHelper();
    
    bool connect();
    void disconnect();
    
    // D-Bus method calling
    DBusMessage* callMethod(const std::string& service, 
                           const std::string& path, 
                           const std::string& interface, 
                           const std::string& method);
    
    DBusMessage* callMethodWithArgs(const std::string& service, 
                                   const std::string& path, 
                                   const std::string& interface, 
                                   const std::string& method,
                                   std::function<void(DBusMessage*)> appendArgs);
    
    // Signal handling
    bool addSignalMatch(const std::string& rule);
    void removeSignalMatch(const std::string& rule);
    
    // Message processing
    void processMessages(int timeoutMs = 1000);
    
    // Property getting/setting
    std::string getStringProperty(const std::string& service,
                                 const std::string& path,
                                 const std::string& interface,
                                 const std::string& property);
    
    bool getBoolProperty(const std::string& service,
                        const std::string& path,
                        const std::string& interface,
                        const std::string& property);
                        
    void setProperty(const std::string& service,
                    const std::string& path,
                    const std::string& interface,
                    const std::string& property,
                    const std::string& value);

private:
    DBusConnection* connection;
    DBusError error;
    
    void initError();
    void checkError();
};

#endif // DBUS_HELPER_H