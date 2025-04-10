/*SSHManager.cpp*/

#include "SSHManager.hpp"
#include <cstdlib>

bool SSHManager::connect_to_ssh(const std::string &hostname, const std::string &password)
{
    ssh_session my_ssh_session = ssh_new();
    if (!my_ssh_session)
    {
        std::cerr << "Error: Unable to create SSH session" << std::endl;
        return false;
    }

    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, hostname.c_str());
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, "root");

    if (ssh_connect(my_ssh_session) != SSH_OK)
    {
        std::cerr << "Error connecting to " << hostname << ": " << ssh_get_error(my_ssh_session) << std::endl;
        ssh_free(my_ssh_session);
        return false;
    }

    if (ssh_userauth_password(my_ssh_session, NULL, password.c_str()) != SSH_AUTH_SUCCESS)
    {
        std::cerr << "Authentication failed: " << ssh_get_error(my_ssh_session) << std::endl;
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return false;
    }

    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    return true;
}

bool SSHManager::create_remote_directory(const std::string &hostname, const std::string &password, const std::string &directory)
{
    std::string command = "sshpass -p '" + password + "' ssh -o StrictHostKeyChecking=no root@" + hostname + " 'mkdir -p " + directory + "'";
    int result = system(command.c_str());

    if (result == 0)
    {
        std::cout << "Successfully created/checked directory: " << directory << " on " << hostname << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to create directory: " << directory << " on " << hostname << std::endl;
        return false;
    }
}

bool SSHManager::scp_transfer(const std::string &hostname, const std::string &password, const std::string &localPath, const std::string &remotePath)
{
    std::string command = "sshpass -p '" + password + "' scp -r " + localPath + " root@" + hostname + ":" + remotePath;
    int result = system(command.c_str());

    if (result == 0)
    {
        std::cout << "Successfully transferred " << localPath << " to " << remotePath << " on " << hostname << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to transfer " << localPath << " to " << remotePath << " on " << hostname << std::endl;
        return false;
    }
}
