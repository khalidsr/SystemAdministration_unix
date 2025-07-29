#ifndef REMOTE_SHELL_HPP
#define REMOTE_SHELL_HPP

#include <string>

class RemoteShell {
public:
    static std::string execute(const std::string& command);
    static bool isAllowed(const std::string& command);
};

#endif