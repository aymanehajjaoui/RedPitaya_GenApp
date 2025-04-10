/*SystemUtils.cpp*/

#include "SystemUtils.hpp"
#include "Common.hpp"
#include <iostream>
#include <sys/statvfs.h>
#include <csignal>
#include <thread>
#include <iomanip>
#include <filesystem>

// Function to check if disk space is below a certain threshold
bool is_disk_space_below_threshold(const char *path, double threshold)
{
    struct statvfs stat;
    if (statvfs(path, &stat) != 0)
    {
        std::cerr << "Error getting filesystem statistics." << std::endl;
        return false;
    }

    double available_space = stat.f_bsize * stat.f_bavail;
    return available_space < threshold;
}

// Set process affinity to a core
void set_process_affinity(int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0)
    {
        std::cerr << "Failed to set process affinity to Core " << core_id << std::endl;
    }
}

bool set_thread_priority(std::thread &th, int priority)
{
    struct sched_param param;
    param.sched_priority = priority;

    if (pthread_setschedparam(th.native_handle(), SCHED_FIFO, &param) != 0)
    {
        std::cerr << "Failed to set thread priority to " << priority << std::endl;
        return false;
    }
    return true;
}

// Function to set thread affinity to a specific core
bool set_thread_affinity(std::thread &th, int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpuset) != 0)
    {
        return false;
    }
    return true;
}

// Signal handler for graceful shutdown
void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        std::cout << "^CSIGINT received, initiating graceful shutdown..." << std::endl;
        stop_program.store(true);
        stop_acquisition.store(true);

        // Forward the signal to child processes to ensure they react
        if (pid1 > 0) kill(pid1, SIGINT);
        if (pid2 > 0) kill(pid2, SIGINT);

        // Notify all threads in current process
        channel1.cond_write_csv.notify_all();
        channel1.cond_model.notify_all();
        channel1.cond_log_csv.notify_all();
        channel1.cond_log_dac.notify_all();
        channel2.cond_write_csv.notify_all();
        channel2.cond_model.notify_all();
        channel2.cond_log_csv.notify_all();
        channel2.cond_log_dac.notify_all();
    }
}

void print_duration(const std::string &label, uint64_t start_ns, uint64_t end_ns)
{
    auto duration_ns = end_ns > start_ns ? end_ns - start_ns : 0;
    auto duration_ms = duration_ns / 1'000'000;

    auto minutes = duration_ms / 60000;
    auto seconds = (duration_ms % 60000) / 1000;
    auto ms = duration_ms % 1000;

    std::cout << std::left << std::setw(40) << label + " acquisition time:"
              << minutes << " min " << seconds << " sec " << ms << " ms\n";
}

void print_channel_stats(const shared_counters_t *counters)
{
    std::cout << "\n====================================\n\n";

    print_duration("Channel 1", counters[0].trigger_time_ns.load(), counters[0].end_time_ns.load());
    print_duration("Channel 2", counters[1].trigger_time_ns.load(), counters[1].end_time_ns.load());

    std::cout << std::left << std::setw(60) << "Total data acquired CH1:" << counters[0].acquire_count.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total lines written CH1:" << counters[0].write_count_csv.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total model calculated CH1:" << counters[0].model_count.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total results logged CH1 to csv file:" << counters[0].log_count_csv.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total results written to DAC_CH1:" << counters[0].log_count_dac.load() << '\n';

    std::cout << std::left << std::setw(60) << "Total data acquired CH2:" << counters[1].acquire_count.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total lines written CH2:" << counters[1].write_count_csv.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total model calculated CH2:" << counters[1].model_count.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total results logged CH2 to csv file:" << counters[1].log_count_csv.load() << '\n';
    std::cout << std::left << std::setw(60) << "Total results written to DAC_CH2:" << counters[1].log_count_dac.load() << '\n';

    std::cout << "\n====================================\n";
}

// Function to manage output folders containing .csv data
void folder_manager(const std::string &folder_path)
{
    namespace fs = std::filesystem;
    
    try {
        fs::path dir_path(folder_path);
        
        if (fs::exists(dir_path)) {
            // Folder exists - clear its contents
            for (const auto& entry : fs::directory_iterator(dir_path)) {
                try {
                    fs::remove_all(entry);
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Failed to delete file: " << entry.path() << " - " << e.what() << std::endl;
                }
            }
        } else {
            // Folder doesn't exist - create it
            if (!fs::create_directories(dir_path)) {
                std::cerr << "Failed to create directory: " << folder_path << std::endl;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

bool ask_user_preferences(bool &save_data, bool &save_output_csv, bool &output_to_dac)
{
    int max_attempts = 3;

    // Ask if the user wants to save acquired data
    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        char save_choice;
        std::cout << "Do you want to save acquired data as CSV files? (y/n): ";
        std::cin >> save_choice;

        if (save_choice == 'y' || save_choice == 'Y')
        {
            save_data = true;
            break;
        }
        else if (save_choice == 'n' || save_choice == 'N')
        {
            save_data = false;
            break;
        }
        else
        {
            std::cerr << "Invalid input. Please type 'y' or 'n'.\n";
            if (attempt == max_attempts)
                return false;
        }
    }

    // Ask what to do with the model output
    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        int output_option;
        std::cout << "\nChoose what to do with model output:\n"
                  << "1. Save as CSV only\n"
                  << "2. Output to DAC only\n"
                  << "3. Both CSV and DAC\n"
                  << "4. None\n"
                  << "Enter your choice (1-4): ";
        std::cin >> output_option;

        if (output_option >= 1 && output_option <= 4)
        {
            save_output_csv = (output_option == 1 || output_option == 3);
            output_to_dac = (output_option == 2 || output_option == 3);
            return true;
        }
        else
        {
            std::cerr << "Invalid input. Please enter a number between 1 and 4.\n";
            if (attempt == max_attempts)
                return false;
        }
    }

    return true;
}


void wait_for_barrier(std::atomic<int>& barrier, int total_participants)
{
    barrier.fetch_add(1);
    while (barrier.load() < total_participants)
        std::this_thread::yield();
}
