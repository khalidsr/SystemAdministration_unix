#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <mutex>
#include "email.hpp"


class Tintin_reporter {
private:
    std::ofstream logFile;

    const std::string logFileName = "matt_daemon.log";
    const std::string logDir = "./"; // or "/var/log/matt_daemon/"
    const size_t maxLogSize = 1024  * 1; // 5 MB


public:
    Tintin_reporter();
    Tintin_reporter(const Tintin_reporter &other);
    Tintin_reporter& operator=(const Tintin_reporter &other) ;
    void rotateLogs(); 
    bool shouldRotate() const;

    bool isOperational() const;
    void log(const std::string &msg, const std::string &type_msg);
    
    ~Tintin_reporter();
};

#endif