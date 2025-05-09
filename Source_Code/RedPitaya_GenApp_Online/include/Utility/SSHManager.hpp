/*SSHManager.hpp*/

#pragma once

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

class SSHManager
{
public:
    static bool connect_to_ssh(const std::string &hostname, const std::string &password, const std::string &privateKeyPath = "");
    static bool create_remote_directory(const std::string &hostname, const std::string &password, const std::string &directory, const std::string &privateKeyPath = "");
    static bool scp_transfer(const std::string &hostname, const std::string &password, const std::string &localPath, const std::string &remotePath, const std::string &privateKeyPath = "");

private:
    static bool authenticate(ssh_session session, const std::string &password, const std::string &privateKeyPath);
    static bool send_directory(ssh_session session, const std::string &localPath, const std::string &remotePath);
    static bool send_file(ssh_session session, const std::filesystem::path &localFilePath, const std::string &remotePath);
    static bool create_remote_directory(ssh_session session, const std::string &remoteDir);
};
