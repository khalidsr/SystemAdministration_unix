#ifndef EMAIL_HPP
#define EMAIL_HPP

#include <string>

class EmailSender 
{
public:
    static void configure(const std::string& to);
    static void sendIfNeeded(const std::string& type, const std::string& msg);


private:
    static std::string recipient;
    static bool shouldSend(const std::string& type);
    static void send(const std::string& subject, const std::string& body);
};

#endif
