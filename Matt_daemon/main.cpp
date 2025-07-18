#include "MattDaemon.hpp"

int main() 
{
    MattDaemon* daemon = MattDaemon::getInstance();
    if (geteuid() != 0) 
    {
        std::cerr << "This program must be run as root.\n";
        return EXIT_FAILURE;
    }

    int result = daemon->run();
    delete daemon;
    return result;
}