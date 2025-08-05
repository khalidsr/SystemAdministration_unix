#include "MattDaemon.hpp"

MattDaemon* MattDaemon::instance = nullptr;

MattDaemon* MattDaemon::getInstance() 
{
    if (instance == nullptr) 
    {
        instance = new MattDaemon();
    }
    return instance;
}

MattDaemon::MattDaemon() : lock_fd(-1), serverSocket(-1) 
{
    activeClients = 0;
    signalPipe[0] = -1; 
    signalPipe[1] = -1;
    shutdownPipe[0] = -1; 
    shutdownPipe[1] = -1;
    instance = this;
}

MattDaemon::~MattDaemon() 
{
    if (lock_fd != -1) 
        close(lock_fd);
    if (serverSocket != -1) 
        close(serverSocket);
  
    if (signalPipe[0] != -1) 
        close(signalPipe[0]);
    if (signalPipe[1] != -1) 
        close(signalPipe[1]);
    if (shutdownPipe[0] != -1) 
        close(shutdownPipe[0]);
    if (shutdownPipe[1] != -1) 
        close(shutdownPipe[1]);
}

void MattDaemon::signal_handler(int signum) 
{
    if (instance) 
    {
        instance->handle_signal(signum);
    }
}

void MattDaemon::handle_signal(int signum) 
{
    char sig = static_cast<char>(signum);
    write(signalPipe[1], &sig, 1);
}

int MattDaemon::create_lockfile() 
{
    lock_fd = open("/var/lock/matt_daemon.lock", O_CREAT | O_RDWR, 0644);
    if (lock_fd < 0) 
    {
        logger.log("Error opening lock file.", "ERROR");
        return -1;
    }
    if (lockf(lock_fd, F_TLOCK, 0) == -1) 
    {
        logger.log("Error: File already locked.", "ERROR");
        return -1;
    }
    write(lock_fd, "Matt Daemon Lock\n", 17);
    return 0;
}

void MattDaemon::daemonize() 
{
    logger.log("Entering Daemon mode.", "INFO");

    pid_t pid = fork();
    if (pid < 0) 
        exit(EXIT_FAILURE);
    if (pid > 0) 
    {
        logger.log("Started. PID: " + std::to_string(pid), "INFO");
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) 
        exit(EXIT_FAILURE);
    
    pid = fork();
    if (pid < 0) 
        exit(EXIT_FAILURE);
    if (pid > 0) 
        exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int MattDaemon::listeningPort() 
{
    logger.log("Creating server.", "INFO");

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) 
    {
        logger.log("Failed to create socket.", "ERROR");
        return -1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4242);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) 
    {
        logger.log("Failed to bind socket.", "ERROR");
        close(serverSocket);
        serverSocket = -1;
        return -1;
    }

    if (listen(serverSocket, 5) < 0) 
    {
        logger.log("Failed to listen on socket.", "ERROR");
        close(serverSocket);
        serverSocket = -1;
        return -1;
    }
    logger.log("Server created.", "INFO");
    daemonize();
    while (true) 
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(serverSocket, &read_fds);
        FD_SET(shutdownPipe[0], &read_fds);
        FD_SET(signalPipe[0], &read_fds);
        int maxfd = std::max(serverSocket, std::max(shutdownPipe[0], signalPipe[0]));

        int ready = select(maxfd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready < 0) 
            continue;

        if (FD_ISSET(signalPipe[0], &read_fds)) 
        {
            char sig;
            read(signalPipe[0], &sig, 1);
            logger.log("Signal received. Quitting.", "INFO");

            for (pid_t pid : clients) 
                kill(pid, SIGKILL);
            while (!clients.empty()) 
            {
                waitpid(*clients.begin(), NULL, 0);
                clients.erase(clients.begin());
            }

            close(serverSocket);
            serverSocket = -1;
            remove("/var/lock/matt_daemon.lock");
            return 0;
        }

        if (FD_ISSET(shutdownPipe[0], &read_fds)) 
        {
            char buf;
            read(shutdownPipe[0], &buf, 1);
            logger.log("Request quit.", "INFO");

            for (pid_t pid : clients) 
                kill(pid, SIGKILL);
            while (!clients.empty())
            {
                waitpid(*clients.begin(), NULL, 0);
                clients.erase(clients.begin());
            }

            close(serverSocket);
            serverSocket = -1;
            remove("/var/lock/matt_daemon.lock");
            return 0;
        }

        if (FD_ISSET(serverSocket, &read_fds)) 
        {
           
            for (auto it = clients.begin(); it != clients.end(); ) 
            {
                if (waitpid(*it, NULL, WNOHANG) > 0) 
                    it = clients.erase(it);
                else 
                    ++it;
            }

            if (activeClients >= 3)
            {
                logger.log("Max clients connected. Rejecting new connection.", "INFO");
                sleep(1);
                continue;
            }

            int clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket < 0) 
                continue;
            activeClients++;
            pid_t pid = fork();
            if (pid < 0) 
            {
                close(clientSocket);
                continue;
            }

            if (pid == 0) 
            {
                close(serverSocket);
                close(shutdownPipe[0]);
                logger.log("Client connected.", "INFO");

                fd_set test_fds;
                FD_ZERO(&test_fds);
                FD_SET(clientSocket, &test_fds);
                struct timeval tv;
                tv.tv_sec = 1; 
                tv.tv_usec = 0;
                
                int ready = select(clientSocket + 1, &test_fds, NULL, NULL, &tv);
                if (ready > 0 && FD_ISSET(clientSocket, &test_fds)) 
                {
                    handleGraphicalClient(clientSocket);
                    exit(0);
                }
            
                if (!handleClientConnection(clientSocket)) 
                {
                    activeClients--;
                    close(clientSocket);
                    exit(0);
                }

                logger.log("Remote shell session started", "INFO");

                char buffer[1024];
                ssize_t bytes_read;
                
                std::string welcome = "MattDaemon Remote Shell - Type 'help' for commands\n";
                write(clientSocket, welcome.c_str(), welcome.size());
                
                while (true) 
                {
                    write(clientSocket, "$ ", 2);
                 
                    bytes_read = read(clientSocket, buffer, sizeof(buffer)-1);
                    if (bytes_read <= 0)
                        break;
                    
                    buffer[bytes_read] = '\0';
                    std::string command = buffer;
                    command.erase(command.find_last_not_of("\r\n") + 1);
                    
                    logger.log("Command: " + command, "LOG");

                    if (command == "quit" || command == "exit") 
                    {
                        write(shutdownPipe[1], "Q", 1);
                        break;
                    }
                    
                    if (command == "help") 
                    {
                        std::string help = "Available commands:\n"
                                        "  help    - Show this help\n"
                                        "  exit    - Exit shell\n"
                                        "  quit    - Quit server\n"
                                        "  allowed commands :[ls, pwd, whoami, date, uname, df, ps," \
                                        "cat, grep, wc, history, id, clear, wich, env ] \n";
                        write(clientSocket, help.c_str(), help.size());
                        continue;
                    }
                    
                    std::string output = RemoteShell::execute(command);
                    write(clientSocket, output.c_str(), output.size());
                }
                
                logger.log("Remote shell session ended", "INFO");
                close(clientSocket);
                exit(0);
            }  
            else 
            { 
                clients.insert(pid);
                close(clientSocket);
            }
        }
    }
    return 0;
}

int MattDaemon::run() 
{
    std::ifstream config("matt_daemon.cfg");
    std::string token;

    if (config.is_open()) 
    {
        std::getline(config, token);

        Authenticator::loadUsers();

        std::string defaultUser = "admin";
        std::string defaultEmail = Authenticator::getEmail(defaultUser);
        EmailSender::configure("hanakhalid57@gmail.com");
        logger.log("Loaded authentication token and email for admin", "INFO");
    }

    else 
    {
        logger.log("Using default authentication token", "WARNING");
    }

    logger.log("Started.", "INFO");

    if (create_lockfile() < 0) 
    {
        logger.log("Failed to create lockfile. Quitting.", "ERROR");
        return EXIT_FAILURE;
    }

    if (pipe(signalPipe) == -1) 
    {
        logger.log("Failed to create signal pipe.", "ERROR");
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    if (pipe(shutdownPipe) == -1) 
    {
        logger.log("Failed to create shutdown pipe.", "ERROR");
        return EXIT_FAILURE;
    }

    int result = listeningPort();

    
    for (int fd : signalPipe)
    {
        if (fd != -1) 
            close(fd);
    }
    for (int fd : shutdownPipe) 
    {
        if (fd != -1) 
        close(fd);
    }
    if (result < 0)
        return EXIT_FAILURE; 
    return EXIT_SUCCESS;
}

void MattDaemon::handleGraphicalClient(int clientSocket) 
{
    char buffer[1024];
    ssize_t bytes_read = read(clientSocket, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) 
    {
        logger.log("Failed to read from graphical client", "WARNING");
        close(clientSocket);
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string message(buffer);
    
    size_t first_colon = message.find(':');
    size_t second_colon = message.find(':', first_colon + 1);
    
    if (first_colon == std::string::npos || second_colon == std::string::npos) 
    {
        logger.log("Invalid message format from graphical client", "WARNING");
        const char* response = "ERROR: Invalid message format. Use username:password:command";
        write(clientSocket, response, strlen(response));
        close(clientSocket);
        return;
    }
    
    std::string username = message.substr(0, first_colon);
    std::string password = message.substr(first_colon + 1, second_colon - first_colon - 1);
    std::string command = message.substr(second_colon + 1);
    
    
    logger.log("Received from GUI client - User: " + username + ", Command: " + command, "INFO");
    if (!Authenticator::authenticate(username, password))
    {
        logger.log("Authentication failed for user: " + username, "WARNING");
        const char* response = "AUTH_FAIL: Invalid credentials";
        write(clientSocket, response, strlen(response));
        close(clientSocket);
        return ;
    }
    
    std::string output = RemoteShell::execute(command);
    if (output.empty()) 
    {
        output = "Command produced no output";
    }
    
    write(clientSocket, output.c_str(), output.size());
    logger.log("Sent response to GUI client", "INFO");
    activeClients--;
    close(clientSocket);
}

bool MattDaemon::handleClientConnection(int clientSocket) 
{
    const char* prompt = "$Enter   << username : password >>\n";
    write(clientSocket, prompt, strlen(prompt));

    char buffer[1024];
    ssize_t bytes_read = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) 
    {
        logger.log("No auth received from CLI client", "WARNING");
        return false;
    }

    buffer[bytes_read] = '\0';
    std::string message(buffer);

    size_t colon = message.find(':');
    if (colon == std::string::npos) 
    {
        const char* fail = "AUTH_FAIL: Expected username:password\n";
        write(clientSocket, fail, strlen(fail));
        return false;
    }

    std::string username = message.substr(0, colon);
    std::string password = message.substr(colon + 1);
    password.erase(password.find_last_not_of("\r\n") + 1);

    if (!Authenticator::authenticate(username, password)) 
    {
        logger.log("Auth failed for CLI user: " + username, "WARNING");
        const char* fail = "AUTH_FAIL: Invalid credentials\n";
        write(clientSocket, fail, strlen(fail));
        return false;
    }

    logger.log("CLI client authenticated as: " + username, "INFO");
    const char* welcome = "AUTH_OK\nMattDaemon Shell - type 'help' or 'exit'\n";
    write(clientSocket, welcome, strlen(welcome));
    
    return true;
}