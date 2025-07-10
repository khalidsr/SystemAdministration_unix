#ifndef MATT_DEAMON_HPP
#define MATT_DEAMON_HPP

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <csignal>
#include <ctime>
#include <iomanip> 

class MattDaemon
{
    private:

    public:
    MattDaemon();
    MattDaemon(const MattDaemon &other);
    MattDaemon& operator=(const MattDaemon &other);
    ~MattDaemon();
};

class Tintin_reporter
{
private:
    std::ofstream logFile;
public:
    Tintin_reporter();
    Tintin_reporter(const Tintin_reporter &other);
    Tintin_reporter& operator=(const Tintin_reporter &other);
    ~Tintin_reporter();
    void log(const std::string &msg, const std::string &type_msg);
};




# endif