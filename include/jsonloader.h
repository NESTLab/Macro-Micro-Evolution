#ifndef __JSON_LOADER__
#define __JSON_LOADER__

#include "syslog.h"

//include rapidjson Parse Library
#include <string>
#include <iostream>
#include <type_traits>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"


namespace json {

    // public
    typedef rapidjson::Value Object;
    extern bool displayErrors;

    // private
    extern bool _ready;
    extern rapidjson::Document _jsonData;

    bool loadProperty(const Object& config, const char* name, Object& parameter);

    template<class T>
    bool loadProperty(const Object& config, const char* name, T& parameter) {
        if(std::is_pointer<T>::value){
            if(displayErrors){
                syslog::cout << "Pointer cannot be loaded from JSON\n";
            }
            return false;
        }

        if(!_ready){
            if(displayErrors){
                syslog::cout << "Must parse JSON data before loading property\n";
            }
            return false;
        }

        if(config.IsObject() && config.HasMember(name)){
            if(std::is_arithmetic<T>::value && config[name].IsNumber()) {
                if(config[name].IsUint()) { parameter = config[name].GetUint(); return true; }
                if(config[name].IsInt()) { parameter = config[name].GetInt(); return true; }

                if(config[name].IsInt64()) { parameter = config[name].GetInt64(); return true; }
                if(config[name].IsUint64()) { parameter = config[name].GetUint64(); return true; }

                if(config[name].IsDouble()) { parameter = config[name].GetDouble(); return true; }
                if(config[name].IsFloat()) { parameter = config[name].GetFloat(); return true; }
            }

            if(config[name].Is<T>()){
                parameter = config[name].Get<T>();
                return true;
            } else {
                if(displayErrors) {
                    syslog::cout << name << " is not a valid type\n";
                }
            }
        }
        return false;
    }

    template<class T>
    bool loadProperty(const char* name, T& parameter) {
        return loadProperty(_jsonData, name, parameter);
    }

    bool parseData(const char* data, size_t length);
    void clearData();

}

#endif // __JSON_LOADER__