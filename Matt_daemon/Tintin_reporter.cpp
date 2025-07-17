#include "Tintin_reporter.hpp"

Tintin_reporter::Tintin_reporter() 
{
    // system("mkdir -p /var/log/matt_daemon");
    
    logFile.open("matt_daemon.log", std::ios::app);
    if (!logFile) {
        std::cerr << "Failed to open log file.\n";
        exit(EXIT_FAILURE);
    }
}

bool Tintin_reporter::isOperational() const 
{
    return logFile.is_open() && logFile.good();
}

void Tintin_reporter::log(const std::string &msg, const std::string &type_msg) 
{
    
    std::time_t now = std::time(nullptr);
    std::tm *lt = std::localtime(&now);
    
    logFile << "[" << std::put_time(lt, "%d/%m/%Y-%H:%M:%S") << "] "
            << "[ " << type_msg << " ] - Matt_daemon: " << msg << std::endl;
            
}

Tintin_reporter::~Tintin_reporter() {
    if (logFile.is_open()) {
        logFile << "Logger shutting down." << std::endl;
        logFile.close();
    }
}