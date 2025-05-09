/*SSHManager.hpp*/

#pragma once

#include <libssh/libssh.h>
#include <iostream>
#include <string>
#include <cstdlib>

class SSHManager
{
public:
    static bool connect_to_ssh(const std::string &hostname, const std::string &password, const std::string &privateKeyPath = "");
    static bool create_remote_directory(const std::string &hostname, const std::string &password, const std::string &directory, const std::string &privateKeyPath = "");
    static bool scp_transfer(const std::string &hostname, const std::string &password, const std::string &localPath, const std::string &remotePath, const std::string &privateKeyPath = "");
};
