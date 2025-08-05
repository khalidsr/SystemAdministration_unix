#include "auth.hpp"
#include <fstream>
#include <sstream>




std::map<std::string, std::string> Authenticator::users;

void Authenticator::loadUsers() 
{
    
    users["admin"] = "admin123";
    users["user1"] = "password1";
    users["user2"] = "password2";
    
    
    std::ifstream file("matt_daemon.cfg");
    if (file.is_open()) 
    {
        std::string line;
        while (std::getline(file, line)) 
        {
            size_t colon = line.find(':');
            if (colon != std::string::npos) 
            {
                std::string user = line.substr(0, colon);
                std::string pass = line.substr(colon + 1);
                users[user] = pass;
            }
        }
    }
}

bool Authenticator::authenticate(const std::string& username, const std::string& password) 
{
    auto it = users.find(username);
    if (it == users.end()) 
        return false;
    return it->second == password;
}

std::string Authenticator::getEmail(const std::string& username) 
{
    auto it = users.find(username);
    if (it != users.end()) 
    {
        return it->second;
    }
    return "";
}
