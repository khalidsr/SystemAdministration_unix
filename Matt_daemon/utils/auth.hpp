#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>
#include <map>

class Authenticator {
public:
    static bool authenticate(const std::string& username, const std::string& password);
    static void loadUsers();
    static std::string getEmail(const std::string& username);

private:
    static std::map<std::string, std::string> users;
};

#endif