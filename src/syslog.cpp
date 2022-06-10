#include "syslog.h"

namespace syslog {

    stream::stream():
        logHandle(nullptr), end(false), logFile("output.log", std::ios::app | std::ios::out | std::ios::in) {
        if(!logFile.is_open()) warning("--- Failed to open log file ---");

        {
            // capture output data to logFile
            logHandle = new std::thread([&](){
                while(!end) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1 second updates
                    std::stringstream copy;
                    {
                        std::scoped_lock lock(mtx);
                        copy << buffer.rdbuf(); // copy data
                    }
                    std::string filter = copy.str();
                    std::cout << filter; // output to console
                    if(logFile.is_open()){ // log to file
                        // filter control headers
                        size_t psize;
                        do {
                            psize = filter.size();
                            std::vector<std::string> filters = {"\33[m", "\33[", "38;2;200;250;200m", "38;2;0;80;190m", "38;2;209;209;16m", "38;2;229;229;16;1m"};
                            for(std::string& r : filters){
                                auto pos = filter.find(r);
                                if(pos != std::string::npos) filter = filter.erase(pos, r.size());
                            }
                        } while(filter.size() != psize);
                        logFile << filter;
                        logFile.flush();
                    }
                }
            });
        }
    }

    stream::~stream() {
        close();
    }

    void stream::close() {
        if(logHandle != nullptr){
            end = true;
            logHandle->join();
            delete logHandle;
            logHandle = nullptr;
        }
        if(logFile.is_open()) logFile.close();
    }

    // public global definition of syslog::cout
    stream cout;

};