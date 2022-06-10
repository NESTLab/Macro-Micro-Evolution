#ifndef __CSV_LOADER_H__
#define __CSV_LOADER_H__

#include "syslog.h"
#include "operators.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class CSVLoader {
    std::string filepath;
    std::ifstream file;
    std::string rawData;
    bool ready;

public:

    CSVLoader();
    CSVLoader(const std::string& path);
    virtual ~CSVLoader();

    void setPath(const std::string& path);
    bool load(Operators::EqPoints& points);

    inline std::string getPath() const { return filepath; }
    inline bool isReady() const { return ready; }

};



#endif //__CSV_LOADER_H__