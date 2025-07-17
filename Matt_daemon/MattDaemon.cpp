#include "MattDaemon.hpp"

MattDaemon* MattDaemon::instance = nullptr;

MattDaemon* MattDaemon::getInstance() {
    if (instance == nullptr) {
        instance = new MattDaemon();
    }
    return instance;
}

MattDaemon::MattDaemon() : lock_fd(-1), serverSocket(-1) {
    // Initialize pipes to invalid descriptors
    signalPipe[0] = -1; signalPipe[1] = -1;
    shutdownPipe[0] = -1; shutdownPipe[1] = -1;
    instance = this;
}

MattDaemon::~MattDaemon() {
    if (lock_fd != -1) {
        close(lock_fd);
    }
    if (serverSocket != -1) {
        close(serverSocket);
    }
    // Clean up pipes
    if (signalPipe[0] != -1) close(signalPipe[0]);
    if (signalPipe[1] != -1) close(signalPipe[1]);
    if (shutdownPipe[0] != -1) close(shutdownPipe[0]);
    if (shutdownPipe[1] != -1) close(shutdownPipe[1]);
}

void MattDaemon::signal_handler(int signum) {
    if (instance) {
        instance->handle_signal(signum);
    }
}

void MattDaemon::handle_signal(int signum) {
    char sig = static_cast<char>(signum);
    write(signalPipe[1], &sig, 1);
}

int MattDaemon::create_lockfile() {
    lock_fd = open("/var/lock/matt_daemon.lock", O_CREAT | O_RDWR, 0644);
    if (lock_fd < 0) {
        logger.log("Error opening lock file.", "ERROR");
        return -1;
    }
    if (lockf(lock_fd, F_TLOCK, 0) == -1) {
        logger.log("Error: File already locked.", "ERROR");
        return -1;
    }
    write(lock_fd, "Matt Daemon Lock\n", 17);
    return 0;
}

void MattDaemon::daemonize() {
    logger.log("Entering Daemon mode.", "INFO");

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) {
        logger.log("Started. PID: " + std::to_string(pid), "INFO");
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) exit(EXIT_FAILURE);
    
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int MattDaemon::listeningPort() {
    logger.log("Creating server.", "INFO");

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        logger.log("Failed to create socket.", "ERROR");
        return -1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(4243);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        logger.log("Failed to bind socket.", "ERROR");
        close(serverSocket);
        serverSocket = -1;
        return -1;
    }

    if (listen(serverSocket, 5) < 0) {
        logger.log("Failed to listen on socket.", "ERROR");
        close(serverSocket);
        serverSocket = -1;
        return -1;
    }

    logger.log("Server created.", "INFO");

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(serverSocket, &read_fds);
        FD_SET(shutdownPipe[0], &read_fds);
        FD_SET(signalPipe[0], &read_fds);
        int maxfd = std::max(serverSocket, std::max(shutdownPipe[0], signalPipe[0]));

        int ready = select(maxfd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready < 0) continue;

        if (FD_ISSET(signalPipe[0], &read_fds)) {
            char sig;
            read(signalPipe[0], &sig, 1);
            logger.log("Signal received. Quitting.", "INFO");

            for (pid_t pid : clients) kill(pid, SIGKILL);
            while (!clients.empty()) {
                waitpid(*clients.begin(), NULL, 0);
                clients.erase(clients.begin());
            }

            close(serverSocket);
            serverSocket = -1;
            remove("/var/lock/matt_daemon.lock");
            return 0;
        }

        if (FD_ISSET(shutdownPipe[0], &read_fds)) {
            char buf;
            read(shutdownPipe[0], &buf, 1);
            logger.log("Shutdown request received.", "INFO");

            for (pid_t pid : clients) kill(pid, SIGKILL);
            while (!clients.empty()) {
                waitpid(*clients.begin(), NULL, 0);
                clients.erase(clients.begin());
            }

            close(serverSocket);
            serverSocket = -1;
            remove("/var/lock/matt_daemon.lock");
            return 0;
        }

        if (FD_ISSET(serverSocket, &read_fds)) {
            // Clean up zombie clients
            for (auto it = clients.begin(); it != clients.end(); ) {
                if (waitpid(*it, NULL, WNOHANG) > 0) 
                    it = clients.erase(it);
                else 
                    ++it;
            }

            if (clients.size() >= 3) {
                logger.log("Max clients connected. Rejecting new connection.", "INFO");
                sleep(1);
                continue;
            }

            int clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket < 0) continue;

            pid_t pid = fork();
            if (pid < 0) {
                close(clientSocket);
                continue;
            }

            if (pid == 0) { // Child process
                close(serverSocket);
                close(shutdownPipe[0]);
                logger.log("Client connected.", "INFO");

                char buffer[1024];
                ssize_t bytes_read;
                while ((bytes_read = read(clientSocket, buffer, sizeof(buffer)-1)))
                 {
                    if (bytes_read <= 0) break;
                    buffer[bytes_read] = '\0';
                    std::string msg = buffer;
                    msg.erase(msg.find_last_not_of("\r\n") + 1);
                    logger.log("User input: " + msg, "LOG");

                    if (msg == "quit") {
                        write(shutdownPipe[1], "Q", 1);
                        break;
                    }
                    std::string response = "Server received: " + msg + "\n";
                    write(clientSocket, response.c_str(), response.size());
                }
                close(clientSocket);
                exit(0);
            } 
            else { // Parent process
                clients.insert(pid);
                close(clientSocket);
            }
        }
    }
    return 0;
}

int MattDaemon::run(int ac, char **av, char **env) {
    (void)ac; (void)av; (void)env; // Unused parameters

    logger.log("Initializing daemon.", "INFO");
    daemonize();

    if (create_lockfile() < 0) {
        logger.log("Failed to create lockfile. Quitting.", "ERROR");
        return EXIT_FAILURE;
    }

    if (pipe(signalPipe) == -1) {
        logger.log("Failed to create signal pipe.", "ERROR");
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    if (pipe(shutdownPipe) == -1) {
        logger.log("Failed to create shutdown pipe.", "ERROR");
        return EXIT_FAILURE;
    }

    int result = listeningPort();

    // Cleanup
    for (int fd : signalPipe) if (fd != -1) close(fd);
    for (int fd : shutdownPipe) if (fd != -1) close(fd);
    
    return (result < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}