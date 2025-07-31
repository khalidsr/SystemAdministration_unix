#ifndef MATT_DAEMON_HPP
#define MATT_DAEMON_HPP
#include "Tintin_reporter.hpp"
#include "utils/auth.hpp"
#include <set>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sys/select.h>
#include <sys/wait.h>
#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include "utils/remote_shell.hpp"
#include "utils/email.hpp"

class MattDaemon 
{
    private:
        static MattDaemon*   instance;
        bool                handleClientConnection(int clientSocket);
        void                handleGraphicalClient(int clientSocket);
        void                loadConfig();
        std::set<pid_t>      clients;
        Tintin_reporter      logger;
        int                  signalPipe[2];
        int                  shutdownPipe[2];
        int                  lock_fd;
        int                  serverSocket;

        MattDaemon();
        MattDaemon(const MattDaemon& other);
        MattDaemon&          operator=(const MattDaemon& other);
        int                 create_server_socket(); 

        static void         signal_handler(int signum);
        void                handle_signal(int signum);
        int                 create_lockfile();
        int                 listeningPort();
        void                daemonize();

    public:
        ~MattDaemon();
        static MattDaemon* getInstance();
        int run();
};

#endif
