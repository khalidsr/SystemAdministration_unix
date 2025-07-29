#include "Tintin_reporter.hpp"

Tintin_reporter::Tintin_reporter() 
{
    // system("mkdir -p /var/log/matt_daemon");
      logFile.open("matt_daemon.log", std::ios::app);
    // logFile.open("/var/log/matt_daemon/matt_daemon.log", std::ios::app);
    if (!logFile) 
    {
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
    if (shouldRotate()) 
        rotateLogs();
        
    std::time_t now = std::time(nullptr);
    std::tm *lt = std::localtime(&now);
    
    logFile << "[" << std::put_time(lt, "%d/%m/%Y-%H:%M:%S") << "] "
            << "[ " << type_msg << " ] - Matt_daemon: " << msg << std::endl;
      
    EmailSender::sendIfNeeded(type_msg, msg);
            
}

Tintin_reporter::~Tintin_reporter() 
{
    
    std::time_t now = std::time(nullptr);
    std::tm *lt = std::localtime(&now);
    if (logFile.is_open())
     {
        logFile << "[" << std::put_time(lt, "%d/%m/%Y-%H:%M:%S") << "] "
            << "[ " << "Info" << " ] - Matt_daemon: "  << "Quitting." << std::endl;
        logFile.close();
    }
}

bool Tintin_reporter::shouldRotate() const {
    std::ifstream in(logDir + logFileName, std::ifstream::ate | std::ifstream::binary);
    return in.tellg() >= static_cast<std::streamoff>(maxLogSize);
}

void Tintin_reporter::rotateLogs() 
{
    if (logFile.is_open())
        logFile.close();

    std::time_t now = std::time(nullptr);
    std::tm *lt = std::localtime(&now);

    std::ostringstream backupName;
    backupName << logDir << "matt_daemon_"
               << std::put_time(lt, "%Y%m%d_%H%M%S") << ".log";

    std::rename((logDir + logFileName).c_str(), backupName.str().c_str());

    // Optional gzip compression
    std::string compressCmd = "gzip " + backupName.str();
    std::system(compressCmd.c_str());

    logFile.open(logDir + logFileName, std::ios::trunc); // New fresh log
}
