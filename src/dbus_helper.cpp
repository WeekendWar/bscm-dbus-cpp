#include "dbus_helper.h"
#include <iostream>
#include <cstring>

DBusHelper::DBusHelper() : connection(nullptr) {
    initError();
}

DBusHelper::~DBusHelper() {
    disconnect();
}

void DBusHelper::initError() {
    dbus_error_init(&error);
}

void DBusHelper::checkError() {
    if (dbus_error_is_set(&error)) {
        std::cerr << "D-Bus error: " << error.message << std::endl;
        dbus_error_free(&error);
    }
}

bool DBusHelper::connect() {
    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    checkError();
    
    if (!connection) {
        std::cerr << "Failed to connect to D-Bus system bus" << std::endl;
        return false;
    }
    
    return true;
}

void DBusHelper::disconnect() {
    if (connection) {
        dbus_connection_unref(connection);
        connection = nullptr;
    }
}

DBusMessage* DBusHelper::callMethod(const std::string& service, 
                                   const std::string& path, 
                                   const std::string& interface, 
                                   const std::string& method) {
    if (!connection) return nullptr;
    
    DBusMessage* msg = dbus_message_new_method_call(
        service.c_str(), path.c_str(), interface.c_str(), method.c_str());
    
    if (!msg) {
        std::cerr << "Failed to create D-Bus message" << std::endl;
        return nullptr;
    }
    
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(
        connection, msg, DBUS_TIMEOUT_USE_DEFAULT, &error);
    
    dbus_message_unref(msg);
    checkError();
    
    return reply;
}

DBusMessage* DBusHelper::callMethodWithArgs(const std::string& service, 
                                           const std::string& path, 
                                           const std::string& interface, 
                                           const std::string& method,
                                           std::function<void(DBusMessage*)> appendArgs) {
    if (!connection) return nullptr;
    
    DBusMessage* msg = dbus_message_new_method_call(
        service.c_str(), path.c_str(), interface.c_str(), method.c_str());
    
    if (!msg) {
        std::cerr << "Failed to create D-Bus message" << std::endl;
        return nullptr;
    }
    
    if (appendArgs) {
        appendArgs(msg);
    }
    
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(
        connection, msg, DBUS_TIMEOUT_USE_DEFAULT, &error);
    
    dbus_message_unref(msg);
    checkError();
    
    return reply;
}

bool DBusHelper::addSignalMatch(const std::string& rule) {
    if (!connection) return false;
    
    dbus_bus_add_match(connection, rule.c_str(), &error);
    checkError();
    
    return !dbus_error_is_set(&error);
}

void DBusHelper::removeSignalMatch(const std::string& rule) {
    if (!connection) return;
    
    dbus_bus_remove_match(connection, rule.c_str(), &error);
    checkError();
}

void DBusHelper::processMessages(int timeoutMs) {
    if (!connection) return;
    
    dbus_connection_read_write_dispatch(connection, timeoutMs);
}

std::string DBusHelper::getStringProperty(const std::string& service,
                                         const std::string& path,
                                         const std::string& interface,
                                         const std::string& property) {
    DBusMessage* reply = callMethodWithArgs(service, path, "org.freedesktop.DBus.Properties", "Get",
        [&](DBusMessage* msg) {
            const char* iface = interface.c_str();
            const char* prop = property.c_str();
            dbus_message_append_args(msg, 
                DBUS_TYPE_STRING, &iface,
                DBUS_TYPE_STRING, &prop,
                DBUS_TYPE_INVALID);
        });
    
    if (!reply) return "";
    
    DBusMessageIter iter, variant_iter;
    if (dbus_message_iter_init(reply, &iter) && 
        dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
        dbus_message_iter_recurse(&iter, &variant_iter);
        if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_STRING) {
            const char* value;
            dbus_message_iter_get_basic(&variant_iter, &value);
            dbus_message_unref(reply);
            return std::string(value);
        }
    }
    
    dbus_message_unref(reply);
    return "";
}

bool DBusHelper::getBoolProperty(const std::string& service,
                                const std::string& path,
                                const std::string& interface,
                                const std::string& property) {
    DBusMessage* reply = callMethodWithArgs(service, path, "org.freedesktop.DBus.Properties", "Get",
        [&](DBusMessage* msg) {
            const char* iface = interface.c_str();
            const char* prop = property.c_str();
            dbus_message_append_args(msg, 
                DBUS_TYPE_STRING, &iface,
                DBUS_TYPE_STRING, &prop,
                DBUS_TYPE_INVALID);
        });
    
    if (!reply) return false;
    
    DBusMessageIter iter, variant_iter;
    if (dbus_message_iter_init(reply, &iter) && 
        dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
        dbus_message_iter_recurse(&iter, &variant_iter);
        if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_BOOLEAN) {
            dbus_bool_t value;
            dbus_message_iter_get_basic(&variant_iter, &value);
            dbus_message_unref(reply);
            return value;
        }
    }
    
    dbus_message_unref(reply);
    return false;
}

void DBusHelper::setProperty(const std::string& service,
                            const std::string& path,
                            const std::string& interface,
                            const std::string& property,
                            const std::string& value) {
    DBusMessage* reply = callMethodWithArgs(service, path, "org.freedesktop.DBus.Properties", "Set",
        [&](DBusMessage* msg) {
            const char* iface = interface.c_str();
            const char* prop = property.c_str();
            const char* val = value.c_str();
            
            DBusMessageIter iter, variant_iter;
            dbus_message_iter_init_append(msg, &iter);
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &iface);
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
            dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &val);
            dbus_message_iter_close_container(&iter, &variant_iter);
        });
    
    if (reply) {
        dbus_message_unref(reply);
    }
}