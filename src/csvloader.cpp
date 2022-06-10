#include "csvloader.h"


CSVLoader::CSVLoader(): ready(false) {
}

CSVLoader::CSVLoader(const std::string& path): filepath(path), ready(false) {}

CSVLoader::~CSVLoader() {}

void CSVLoader::setPath(const std::string& path){
    filepath = path;
}

bool CSVLoader::load(Operators::EqPoints& points) {
    file.open(filepath, std::ios::in | std::ios::binary); // open file
    if(!file.is_open()){
        syslog::cout << "error opening: failed to open the file\n";
        return false;
    }
    { // cache buffer scope
        std::stringstream cache; // temporary buffer
        file >> cache.rdbuf(); // dump data into cache buffer
        rawData = cache.str(); // extract data from cache buffer
    }
    file.close(); // close file when finished
    
    size_t pos = 0, row = 0;
    std::vector<std::vector<VTYPE>> inpoints;
    inpoints.resize(1, {});
    std::string part;

    auto addPart = [&]() -> bool {
        if(!part.empty()){
            try {
                double val = std::stod(part);
                inpoints[row].push_back(val);
            } catch (std::invalid_argument e) {
                syslog::cout << "invalid_argument error: " << part << " at position " << pos << "\n";
                return false;
            }
            part.clear();
            return true;
        }
        return true; // pass - no data
    };

    do {
        char c = rawData[pos++];
        switch(c){
            case ' ':
            case '\r':
            case '\t':{
                continue;
            }
            case ',': {
                if(!addPart()) return false;
                continue;
            }
            case '\n':{
                if(part.empty()) continue;

                if(!addPart()) return false;
                inpoints.push_back({});
                row++;
                continue;
            }
        }
        if((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != 'e'){
            syslog::cout << "csv syntax error: unexpected symbol " << c << " [" << int(uint8_t(c)) << "] @ position " << pos << "!\n";
            return false; // invalid format
        }

        part += c;
    } while(pos < rawData.size());
    if(!part.empty()) { // get last part in case no line or comma was added
        if(!addPart()) return false;
    }
    inpoints.pop_back(); // remove last row which is invalid

    size_t len = inpoints[0].size(), i=0;
    
    points.numVars = len - 1; // last column holds result - number of vars will be the column count - 1

    for(auto& row : inpoints){ // check for all rows to have the same size
        if(len != row.size()){
            syslog::cout << "point cloud error: the row sizes do not match!\n";
            return false;
        }
        ++i;
    }
    std::vector<VTYPE> empty; // a blank / empty container of blank points
    empty.resize(points.numVars, 0);
    points.points.resize(inpoints.size(), empty); // preset the points with empty columns+rows
    points.results.resize(inpoints.size(), 0.0); // preset the results

    for(size_t row = 0; row < inpoints.size(); row++){
        for(size_t col = 0; col < inpoints[row].size(); col++){
            if(col == points.numVars){ // result value
                points.results[row] = inpoints[row][col]; // copy result
            } else { // variable value
                points.points[row][col] = inpoints[row][col]; // copy variable
            }
        }
    }

    ready = true;
    return true;
}