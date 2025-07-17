// #include "MattDaemon.hpp"

// #include <set>
// #include <sys/wait.h>

// std::set<pid_t> clients;
// Tintin_reporter *g_logger = nullptr;

// void signal_handler(int signum) 
// {
    
   
//     if (g_logger && getppid() != 1) 
//     {
//         g_logger->log("Signal handler.", "INFO");
//         g_logger->log("Quitting.", "INFO");
//     }

//     for (pid_t pid : clients) 
//     {
//         kill(pid, SIGTERM);
//     }
    
//     exit(0);
// }

// int create_lockfile(Tintin_reporter &logger) 
// {
//   std::ofstream outFile;
//     outFile.open("/var/lock/matt_daemon.lock", std::ios::out);
//     if (outFile.is_open()) 
//     {
//         outFile << "Hello, C++98 File Handling!" << std::endl;
//         outFile.close();
//         std::cout << "Data written to example.txt" << std::endl;
//     } 
//     else 
//     {
//         logger.log("Error file locked.", "ERROR");
//         std::cerr << "Error opening file!" << std::endl;
//         return -1;
//     }

//     // int lock_fd = open("matt_daemon.lock", O_CREAT | O_RDWR, 0644);
//     // if (lock_fd < 0) {
//     //     logger.log("Failed to create/open lock file.");
//     //     return -1;
//     // }

//     // if (lockf(lock_fd, F_TLOCK, 0) < 0) {
//     //     logger.log("Another daemon instance is already running.");
//     //     return -1;
//     // }

//     return 0;
// }
// int listeningPort(Tintin_reporter &logger) 
// {
//     logger.log("Creating server.", "INFO");
//     g_logger = &logger;
   
//     std::signal(SIGINT, signal_handler);
//     std::signal(SIGTERM, signal_handler);
//     std::signal(SIGHUP, signal_handler);
    
//     int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverSocket == -1) {
//         logger.log("Failed to create socket.", "INFO");
//         std::cerr << "Error creating socket\n";
//         return -1;
//     }

//     int opt = 1;
//     if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) 
//     {
//         logger.log("Failed to set socket options.", "INFO");
//         std::cerr << "Error setting SO_REUSEADDR\n";
//         close(serverSocket);
//         return -1;
//     }

//     sockaddr_in serverAddress;
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_port = htons(4243);
//     serverAddress.sin_addr.s_addr = INADDR_ANY;

//     if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)))
//      {
//         logger.log("Failed to bind socket.", "INFO");
//         std::cerr << "Error binding socket\n";
//         close(serverSocket);
//         return -1;
//     }

//     if (listen(serverSocket, 5) == -1) 
//     {
//         logger.log("Failed to listen on socket.", "INFO");
//         std::cerr << "Error listening on socket\n";
//         close(serverSocket);
//         return -1;
//     }

//     logger.log("Server created.", "INFO");

//     std::cout << "Server listening on port 4243 ...\n";
    
//     while (true)
//     {
//         for (auto it = clients.begin(); it != clients.end(); )
//         {
//             if (waitpid(*it, nullptr, WNOHANG) > 0)
//                 it = clients.erase(it);
//             else
//                 ++it;
//         }

//         if (clients.size() >= 3)
//         {
//             std::cout << "Max clients connected. Waiting for a slot...\n";
//             sleep(1);
//             continue;
//         }

//         int clientSocket = accept(serverSocket, nullptr, nullptr);
//         if (clientSocket == -1) 
//         {
//             logger.log("Failed to accept client connection.", "INFO");
//             continue;
//         }

//         pid_t pid = fork();

//         if (pid < 0) 
//         {
//             logger.log("Failed to fork for client.", "ERROR");
//             close(clientSocket);
//             continue;
//         }

//         if (pid == 0) 
//         {
           
//             close(serverSocket);
        
//             logger.log("Client connected.", "INFO");

//             while (true)
//             {
//                 char buffer[1024] = {0};
//                 ssize_t bytes_read = read(clientSocket, buffer, sizeof(buffer) - 1);

//                 if (bytes_read <= 0)
//                 {
//                     std::cerr << "Client disconnected or read failed.\n";
//                     break;
//                 }

//                 buffer[bytes_read] = '\0';
//                 std::string msg(buffer);
//                 msg.erase(msg.find_last_not_of("\r\n") + 1);

//                 logger.log("User input: " + msg, "LOG");
//                 std::cout << "Received: " << msg << "\n";

//                 if (msg == "quit")
//                 {
//                     logger.log("Request quit.", "INFO");
//                     logger.log("Quitting.", "INFO");
//                     std::cout << "Quit command received. Closing connection.\n";
//                     kill(getppid(), SIGTERM);
//                     remove("/var/lock/matt_daemon.lock");
//                     close(clientSocket);
//                     break; 
//                     // exit(0);                  
//                 }

//                 std::string response = "Server received: " + msg + "\n";
//                 write(clientSocket, response.c_str(), response.length());
//             }            

//             close(clientSocket);
//             exit(0);
//         }
//         else
//         {
            
//             clients.insert(pid);
//             close(clientSocket);
//         }
//     }

//     close(serverSocket);
//     return 0;
// }

// void daemonize(Tintin_reporter &logger) 
// {
//     logger.log("Entering Daemon mode.", "INFO");

//     pid_t pid = fork();
//     std::cout << "=============" << pid << std::endl;

//     if (pid < 0) 
//         exit(EXIT_FAILURE);

//     if (pid > 0) 
//     {
//         logger.log("started. PID: " + std::to_string(pid), "INFO");
//         exit(EXIT_SUCCESS);  
//     }

//     setsid();      
//     // chdir("/");       
//     close(STDIN_FILENO);
//     close(STDOUT_FILENO);
//     close(STDERR_FILENO);
// }



// int main(int ac, char **av, char **env)
// {
//     std::ofstream outFile;
//     Tintin_reporter logger;
//     // if (geteuid() != 0) 
//     // {
//     //     std::cerr << "This program must be run as root.\n";
//     //     return EXIT_FAILURE;
//     // }

//     // std::cout << "Running as root...\n";

//     logger.log("Started.", "INFO");
//     int res = create_lockfile(logger);
//     if (res < 0)
//       return EXIT_FAILURE;
//     daemonize(logger);
//     int result = listeningPort(logger);
//     if (result < 0)
//       return EXIT_FAILURE;
    
//     std::cout << "Server shutting down.\n";
//     return 0;
// }