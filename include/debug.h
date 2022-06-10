#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>
#include <string>
#include "syslog.h"

#define CSI "\33["
#define RESET "\33[m"

void debug(double, bool important=false);
void debug(const std::string& str = "", bool important=false);
void warning(const std::string& str = "");

class Debugger {
    static bool enabled, pretty, verbose;
    static size_t counter;

    Debugger() = delete;
    virtual ~Debugger() = delete;

public:
    static void beginDebug();
    static inline void setEnable(bool en) { enabled = en; }
    static inline void setVerbose(bool en) { verbose = en; }
    static inline void resetCounter() { counter = 0; }

    friend void debug(double, bool important);
    friend void debug(const std::string&, bool important);
    friend void warning(const std::string&);
};

#endif  // __DEBUG_H__