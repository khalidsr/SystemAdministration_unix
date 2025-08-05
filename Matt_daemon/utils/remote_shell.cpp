#include "remote_shell.hpp"
#include <array>
#include <memory>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>


const char* ALLOWED_COMMANDS[] = 
{
    "ls", "pwd", "whoami", "date", "uname", "df", "ps",
    "cat", "grep", "wc","history","id","clear","wich","env", nullptr  
};

bool RemoteShell::isAllowed(const std::string& command) 
{
    for (const char** allowed = ALLOWED_COMMANDS; *allowed; ++allowed) 
    {
        if (command.find(*allowed) == 0) 
        {
            
            if (command.size() == strlen(*allowed) || command[strlen(*allowed)] == ' ') 
                return true;
        }
    }
    return false;
}

std::string RemoteShell::execute(const std::string& command) 
{
    if (!isAllowed(command))
        return "Error: Command not allowed\n";

    int pipefd[2];
    if (pipe(pipefd) == -1)
        return "Error: Failed to create pipe\n";

    pid_t pid = fork();
    if (pid == -1) 
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return "Error: Failed to fork\n";
    }

    if (pid == 0) 
    { 
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);
        exit(127);
    } 
    else 
    {  
        close(pipefd[1]);
        std::string result;
        char buffer[256];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) 
        {
            buffer[count] = '\0';
            result += buffer;
        }

        int status;
        waitpid(pid, &status, 0);
        close(pipefd[0]);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) 
        {
            result += "Command exited with non-zero status\n";
        }

        return result;
    }
}