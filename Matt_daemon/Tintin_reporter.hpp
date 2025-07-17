#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <mutex>  

class Tintin_reporter {
private:
    std::ofstream logFile;

public:
    Tintin_reporter();
    Tintin_reporter(const Tintin_reporter &other);
    Tintin_reporter& operator=(const Tintin_reporter &other) ;
    
    bool isOperational() const;
    void log(const std::string &msg, const std::string &type_msg);
    
    ~Tintin_reporter();
};

#endif