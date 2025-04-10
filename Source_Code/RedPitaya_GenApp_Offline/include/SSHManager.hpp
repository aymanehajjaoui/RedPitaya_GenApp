/*SSHManager.hpp*/

#pragma once

#include <libssh/libssh.h>
#include <iostream>
#include <string>

class SSHManager
{
public:
    static bool connect_to_ssh(const std::string &hostname, const std::string &password);
    static bool create_remote_directory(const std::string &hostname, const std::string &password, const std::string &directory);
    static bool scp_transfer(const std::string &hostname, const std::string &password, const std::string &localPath, const std::string &remotePath);
};

