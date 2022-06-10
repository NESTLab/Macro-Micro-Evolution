#include "debug.h"

#ifdef _WIN32
    #include "windows.h"
#endif

bool Debugger::enabled = false;
size_t Debugger::counter = 0;
bool Debugger::pretty = false;
bool Debugger::verbose = false;

void Debugger::beginDebug() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(console, &mode);
    
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    mode |= DISABLE_NEWLINE_AUTO_RETURN;

    if(SetConsoleMode(console, mode)) pretty = true;
#else
    pretty = true;
#endif
    setEnable(true);
}

void debug(double n, bool important) {
    debug(std::to_string(n), important);
}

void debug(const std::string& str, bool important) {
    if((!important && !Debugger::verbose) || !Debugger::enabled) return;

    if(Debugger::pretty) syslog::cout << CSI << "38;2;0;80;190m";
    syslog::cout << "[" << (Debugger::counter++) << "] ";
    if(Debugger::pretty) syslog::cout << RESET << CSI << "38;2;200;250;200m";
    syslog::cout << str << "\n";
    if(Debugger::pretty) syslog::cout << RESET;
}

void warning(const std::string& str) {
    if(Debugger::pretty) syslog::cout << CSI << "38;2;229;229;16;1m";
    syslog::cout << "WARNING: ";
    if(Debugger::pretty) syslog::cout << RESET << CSI << "38;2;209;209;16m";
    syslog::cout << str;
    if(Debugger::pretty) syslog::cout << RESET;
    syslog::cout << "\n";
}