#ifndef __SYSLOG_H_
#define __SYSLOG_H_

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include <thread>
#include <mutex>
#include <atomic>

#include "debug.h"

namespace syslog {

    class stream {
        std::stringstream buffer;
        std::mutex mtx;
        std::thread* logHandle;
        std::atomic<bool> end; // end log handle
        std::fstream logFile;

    public:

        stream();
        virtual ~stream();

        void close();

        template<typename T>
        inline stream& operator<<(T&& value) {
            std::scoped_lock lock(mtx);
            buffer << value;
            return *this;
        }

    };

    extern stream cout;

};

#endif // __SYSLOG_H_