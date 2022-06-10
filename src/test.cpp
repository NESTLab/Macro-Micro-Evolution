#include "node.h"
#include "clock.h"
#include "debug.h"
#include "evoalgo.h"
#include "visualevo.h"
#include "csvloader.h"
#include "syslog.h"


#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <atomic>
#include <thread>

using namespace NodeTypes;


int main(int argc, char** argv) {
    Debugger::beginDebug();

    // load program arguments
    std::vector<std::string> arguments;
    for(int i=0; i < argc; ++i){
        std::string arg(argv[i]);
        arguments.emplace_back( arg );
    }

    Clock time;

    Parameters::Init();


    std::string currentTime; // string that contains start time as pretty string
    {
        const time_t _tdata = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        currentTime = ctime( &_tdata );
        currentTime = currentTime.substr(0, currentTime.size() - 1);
    }

    syslog::cout << "------------ Program Start @ " << currentTime << " ------------------------" << "\n";
    syslog::cout << "System has " << std::thread::hardware_concurrency() << " available hardware threads" << "\n";
    
    Parameters::Load("config.json"); // load default config json file

    Parameters* p = Parameters::Params(); // initialize parameters
    VisualEvo* evoWin = nullptr;

    Debugger::setVerbose(p->verboseLogging); // update verbose logging

    { // run the program
        syslog::cout << "Initializing..." << "\n";
        Operators::EqPoints& data = p->points; // get reference to the already instantiated instance - modify p->points

        CSVLoader* inputData = new CSVLoader;
    
        if(arguments.size() > 1) {
            inputData->setPath(arguments[1]);
        } else {
            inputData->setPath(p->defaultPointCloudCSV);
        }

        if(!inputData->load(data)){
            warning("Failed to load \"" + inputData->getPath() + "\" point cloud data");
        }

        if(inputData != nullptr){
            delete inputData;
            inputData = nullptr;
        }
        
        syslog::cout << "Starting EvoAlgo..." << "\n";
        
        if(data.results.empty()){ // determine if program can actually run
            warning("No point cloud data loaded! Must provide point cloud data to start program");
        } else {
            // sort point cloud data via first variable (var0) as x
            for(size_t j=0;j < data.points.size() - 1; ++j){
                for(size_t i=j+1;i < data.points.size(); ++i){
                    if(data.points[i][0] < data.points[j][0]){
                        std::swap(data.points[i], data.points[j]);
                        std::swap(data.results[i], data.results[j]);
                    }
                }
            }

            if(p->visual.display){
                evoWin = new VisualEvo(580, 580, p->visual.xresolution, p->visual.yresolution);
            }

            if(evoWin != nullptr){
                Clock timer;
                do {
                    if(timer.getSeconds() > 5){
                        delete evoWin;
                        evoWin = nullptr;
                        break;
                    }
                } while(!evoWin->isRunning()); // wait 5 seconds for window to populate

                if(evoWin != nullptr){
                    p->visual.graph = evoWin->createGraph(p->visual.xscale, p->visual.yscale);
                } else {
                    warning("Failed to start VisualEvo because there might not be a display available.\nIf you wish to run without VisualEvo, please disable VisualEvo in the config file.");
                }
            }

            EvoAlgo evo(p, data); // auto loads parameters and data

            evo.run();

            if(evoWin != nullptr){
                if(!p->visual.closeOnFinish) while(evoWin->isRunning()); // wait for user to close window
                delete evoWin;
            }
        }

    }
    
    Parameters::Free();

    {   // get run time for entire program
        double len = time.getMilliseconds();
        
        std::string unit = "ms";
        if(len >= 100000){
			len /= 1000.; unit = "sec";
			if(len >= 1000){ 
				len /= 60.; unit = "min"; 
				if(len >= 200){
					len /= 60.; unit = "hr";
					if(len >= 24){
						len /= 24.; unit = "day";
					}
				}
			}
		}
        
        syslog::cout << "---------- Program finished at " << len << unit << " -------------" <<"\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(3)); // wait 3 seconds for log to flush

	return 0;
}
