/* SSHManager.cpp */

#include "Utility/SSHManager.hpp"

bool SSHManager::connect_to_ssh(const std::string &hostname, const std::string &password, const std::string &privateKeyPath)
{
    ssh_session session = ssh_new();
    if (!session)
    {
        std::cerr << "Error: Unable to create SSH session" << std::endl;
        return false;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, hostname.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, "root");

    if (ssh_connect(session) != SSH_OK)
    {
        std::cerr << "Error connecting to " << hostname << ": " << ssh_get_error(session) << std::endl;
        ssh_free(session);
        return false;
    }

    bool authSuccess = authenticate(session, password, privateKeyPath);
    ssh_disconnect(session);
    ssh_free(session);
    return authSuccess;
}

bool SSHManager::authenticate(ssh_session session, const std::string &password, const std::string &privateKeyPath)
{
    int rc;
    if (!privateKeyPath.empty())
    {
        ssh_key privkey = nullptr;
        rc = ssh_pki_import_privkey_file(privateKeyPath.c_str(), nullptr, nullptr, nullptr, &privkey);
        if (rc != SSH_OK)
        {
            std::cerr << "Failed to load private key: " << ssh_get_error(session) << std::endl;
            return false;
        }

        rc = ssh_userauth_publickey(session, nullptr, privkey);
        ssh_key_free(privkey);
    }
    else
    {
        rc = ssh_userauth_password(session, nullptr, password.c_str());
    }

    if (rc != SSH_AUTH_SUCCESS)
    {
        std::cerr << "Authentication failed: " << ssh_get_error(session) << std::endl;
        return false;
    }

    return true;
}

bool SSHManager::create_remote_directory(const std::string &hostname, const std::string &password, const std::string &directory, const std::string &privateKeyPath)
{
    ssh_session session = ssh_new();
    if (!session)
    {
        std::cerr << "Error: Unable to create SSH session" << std::endl;
        return false;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, hostname.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, "root");

    if (ssh_connect(session) != SSH_OK)
    {
        std::cerr << "Error connecting to " << hostname << ": " << ssh_get_error(session) << std::endl;
        ssh_free(session);
        return false;
    }

    if (!authenticate(session, password, privateKeyPath))
    {
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }

    bool result = create_remote_directory(session, directory);  

    ssh_disconnect(session);
    ssh_free(session);
    return result;
}

bool SSHManager::create_remote_directory(ssh_session session, const std::string &remoteDir)
{
    ssh_channel channel = ssh_channel_new(session);
    if (!channel)
    {
        std::cerr << "Failed to create channel (mkdir)" << std::endl;
        return false;
    }

    if (ssh_channel_open_session(channel) != SSH_OK)
    {
        ssh_channel_free(channel);
        return false;
    }

    std::string cmd = "mkdir -p \"" + remoteDir + "\"";
    if (ssh_channel_request_exec(channel, cmd.c_str()) != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return false;
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return true;
}

bool SSHManager::scp_transfer(const std::string &hostname, const std::string &password, const std::string &localPath, const std::string &remotePath, const std::string &privateKeyPath)
{
    ssh_session session = ssh_new();
    if (!session)
    {
        std::cerr << "Error: Unable to create SSH session" << std::endl;
        return false;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, hostname.c_str());
    ssh_options_set(session, SSH_OPTIONS_USER, "root");

    if (ssh_connect(session) != SSH_OK)
    {
        std::cerr << "Error connecting to " << hostname << ": " << ssh_get_error(session) << std::endl;
        ssh_free(session);
        return false;
    }

    if (!authenticate(session, password, privateKeyPath))
    {
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }

    bool success = false;
    if (std::filesystem::is_directory(localPath))
    {
        success = send_directory(session, localPath, remotePath);
    }
    else if (std::filesystem::is_regular_file(localPath))
    {
        success = send_file(session, localPath, remotePath);
    }
    else
    {
        std::cerr << "Invalid local path: " << localPath << std::endl;
    }

    ssh_disconnect(session);
    ssh_free(session);
    return success;
}

bool SSHManager::send_directory(ssh_session session, const std::string &localPath, const std::string &remotePath)
{
    for (const auto &entry : std::filesystem::recursive_directory_iterator(localPath))
    {
        if (entry.is_regular_file())
        {
            std::string relativePath = std::filesystem::relative(entry.path(), localPath).string();
            std::string targetPath = remotePath + "/" + relativePath;
            std::string remoteDir = std::filesystem::path(targetPath).parent_path().string();

            if (!create_remote_directory(session, remoteDir))
            {
                std::cerr << "Failed to create remote directory: " << remoteDir << std::endl;
                return false;
            }

            if (!send_file(session, entry.path(), targetPath))
            {
                std::cerr << "Failed to send file: " << entry.path() << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool SSHManager::send_file(ssh_session session, const std::filesystem::path &localFilePath, const std::string &remotePath)
{
    ssh_scp scp = ssh_scp_new(session, SSH_SCP_WRITE, std::filesystem::path(remotePath).parent_path().c_str());
    if (!scp)
    {
        std::cerr << "Error allocating SCP: " << ssh_get_error(session) << std::endl;
        return false;
    }

    if (ssh_scp_init(scp) != SSH_OK)
    {
        std::cerr << "Error initializing SCP: " << ssh_get_error(session) << std::endl;
        ssh_scp_free(scp);
        return false;
    }

    std::ifstream file(localFilePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open local file: " << localFilePath << std::endl;
        ssh_scp_free(scp);
        return false;
    }

    auto fileSize = std::filesystem::file_size(localFilePath);
    auto filename = localFilePath.filename().string();

    if (ssh_scp_push_file(scp, filename.c_str(), fileSize, S_IRUSR | S_IWUSR) != SSH_OK)
    {
        std::cerr << "Can't open remote file: " << ssh_get_error(session) << std::endl;
        file.close();
        ssh_scp_free(scp);
        return false;
    }

    std::vector<char> buffer(4096);
    while (file)
    {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0)
        {
            if (ssh_scp_write(scp, buffer.data(), bytesRead) != SSH_OK)
            {
                std::cerr << "Error writing to remote file: " << ssh_get_error(session) << std::endl;
                file.close();
                ssh_scp_free(scp);
                return false;
            }
        }
    }

    file.close();
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    return true;
}
