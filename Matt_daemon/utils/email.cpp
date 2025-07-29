#include "email.hpp"
#include <cstdlib>
#include <iostream>

std::string EmailSender::recipient = "";

void EmailSender::configure(const std::string& to) 
{
    recipient = to;
}

bool EmailSender::shouldSend(const std::string& type) 
{
    return type == "ERROR" || type == "WARNING";
}

void EmailSender::sendIfNeeded(const std::string& type, const std::string& msg) {
    if (recipient.empty()) return;
    if (!shouldSend(type)) return;

    std::string subject = "[MattDaemon] " + type + " Alert";
    send(subject, msg);
}

void EmailSender::send(const std::string& subject, const std::string& body) 
{
    std::string cmd =
        "echo \"" + body + "\" | mail -s \"" + subject + "\" \"" + recipient + "\"";
    std::cout<<"=========="<<cmd<<std::endl;
    int ret = system(cmd.c_str());

    if (ret != 0)
        std::cerr << "Failed to send email\n";
}
