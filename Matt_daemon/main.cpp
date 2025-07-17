#include "MattDaemon.hpp"

int main(int ac, char **av, char **env) {
    MattDaemon* daemon = MattDaemon::getInstance();
    int result = daemon->run(ac, av, env);
    delete daemon; // Clean up singleton instance
    return result;
}