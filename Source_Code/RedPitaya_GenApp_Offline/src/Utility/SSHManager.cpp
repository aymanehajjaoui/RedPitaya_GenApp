/*SSHManager.cpp*/

#include "Utility/SSHManager.hpp"

bool SSHManager::connect_to_ssh(const std::string &hostname, const std::string &password, const std::string &privateKeyPath)
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

    int authResult;
    if (!privateKeyPath.empty())
    {
        ssh_key privateKey = nullptr;
        int rc = ssh_pki_import_privkey_file(privateKeyPath.c_str(), nullptr, nullptr, nullptr, &privateKey);
        if (rc != SSH_OK)
        {
            std::cerr << "Failed to load private key: " << ssh_get_error(my_ssh_session) << std::endl;
            ssh_disconnect(my_ssh_session);
            ssh_free(my_ssh_session);
            return false;
        }

        authResult = ssh_userauth_publickey(my_ssh_session, nullptr, privateKey);
        ssh_key_free(privateKey);
    }
    else
    {
        authResult = ssh_userauth_password(my_ssh_session, nullptr, password.c_str());
    }

    if (authResult != SSH_AUTH_SUCCESS)
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

bool SSHManager::create_remote_directory(const std::string &hostname, const std::string &password, const std::string &directory, const std::string &privateKeyPath)
{
    std::string command;
    if (!privateKeyPath.empty())
        command = "ssh -i " + privateKeyPath + " -o StrictHostKeyChecking=no root@" + hostname + " 'mkdir -p " + directory + "'";
    else
        command = "sshpass -p '" + password + "' ssh -o StrictHostKeyChecking=no root@" + hostname + " 'mkdir -p " + directory + "'";

    return system(command.c_str()) == 0;
}

bool SSHManager::scp_transfer(const std::string &hostname, const std::string &password, const std::string &localPath, const std::string &remotePath, const std::string &privateKeyPath)
{
    std::string command;
    if (!privateKeyPath.empty())
        command = "scp -i " + privateKeyPath + " -o StrictHostKeyChecking=no -r " + localPath + " root@" + hostname + ":" + remotePath;
    else
        command = "sshpass -p '" + password + "' scp -o StrictHostKeyChecking=no -r " + localPath + " root@" + hostname + ":" + remotePath;

    return system(command.c_str()) == 0;
}
