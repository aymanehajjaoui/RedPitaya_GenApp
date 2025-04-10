/* main.cpp */

#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <deque>
#include <csignal>
#include <chrono>
#include <sched.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iomanip>
#include "rp.h"
#include "Common.hpp"
#include "SystemUtils.hpp"
#include "DataAcquisition.hpp"
#include "DataWriter.hpp"
#include "ModelProcessing.hpp"
#include "ModelWriter.hpp"
#include "DAC.hpp"

pid_t pid1 = -1;
pid_t pid2 = -1;

int main()
{
    if (rp_Init() != RP_OK)
    {
        std::cerr << "Rp API init failed!" << std::endl;
        return -1;
    }

    std::signal(SIGINT, signal_handler);

    folder_manager("DataOutput");
    folder_manager("ModelOutput");

    int shm_fd_counters = shm_open(SHM_COUNTERS, O_CREAT | O_RDWR, 0666);
    if (shm_fd_counters == -1)
    {
        std::cerr << "Error creating shared memory for counters!" << std::endl;
        return -1;
    }
    if (ftruncate(shm_fd_counters, sizeof(shared_counters_t) * 2) == -1)
    {
        std::cerr << "Error setting shared memory size for counters!" << std::endl;
        return -1;
    }

    shared_counters_t *shared_counters = (shared_counters_t *)mmap(
        0, sizeof(shared_counters_t) * 2, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_counters, 0);
    if (shared_counters == MAP_FAILED)
    {
        std::cerr << "Shared memory mapping for counters failed!" << std::endl;
        return -1;
    }

    new (&shared_counters[0].acquire_count) std::atomic<int>(0);
    new (&shared_counters[0].model_count) std::atomic<int>(0);
    new (&shared_counters[0].write_count_csv) std::atomic<int>(0);
    new (&shared_counters[0].log_count_csv) std::atomic<int>(0);
    new (&shared_counters[0].log_count_dac) std::atomic<int>(0);

    new (&shared_counters[1].acquire_count) std::atomic<int>(0);
    new (&shared_counters[1].model_count) std::atomic<int>(0);
    new (&shared_counters[1].write_count_csv) std::atomic<int>(0);
    new (&shared_counters[1].log_count_csv) std::atomic<int>(0);
    new (&shared_counters[1].log_count_dac) std::atomic<int>(0);

    new (&shared_counters[0].ready_barrier) std::atomic<int>(0);
    new (&shared_counters[1].ready_barrier) std::atomic<int>(0);

    std::cout << "Starting program" << std::endl;

    bool save_data = false, save_output_csv = false, output_to_dac = false;
    if (!ask_user_preferences(save_data, save_output_csv, output_to_dac))
    {
        std::cerr << "User input failed. Exiting." << std::endl;
        return -1;
    }

    initialize_acq();
    initialize_DAC();

    pid1 = fork();

    if (pid1 < 0)
    {
        std::cerr << "Fork for CH1 failed!" << std::endl;
        return -1;
    }
    else if (pid1 == 0)
    {
        std::cout << "Child Process 1 (CH1) started. PID: " << getpid() << std::endl;

        int shm_fd_counters_ch1 = shm_open(SHM_COUNTERS, O_RDWR, 0666);
        shared_counters_t *shared_counters_ch1 = (shared_counters_t *)mmap(
            0, sizeof(shared_counters_t) * 2, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_counters_ch1, 0);
        if (shared_counters_ch1 == MAP_FAILED)
        {
            std::cerr << "Shared memory mapping failed in CH1!" << std::endl;
            exit(-1);
        }

        channel1.counters = &shared_counters_ch1[0];
        set_process_affinity(0);

        wait_for_barrier(shared_counters_ch1[0].ready_barrier, 2);
        std::thread acq_thread(acquire_data, std::ref(channel1), RP_CH_1);
        std::thread model_thread(model_inference, std::ref(channel1));

        std::thread log_thread, write_thread, dac_thread;

        if (save_output_csv)
            log_thread = std::thread(log_results, std::ref(channel1), "ModelOutput/output_ch1.csv");
        if (output_to_dac)
            dac_thread = std::thread(dac_output_loop, std::ref(channel1), RP_CH_1);
        if (save_data)
            write_thread = std::thread(write_data, std::ref(channel1), "DataOutput/data_ch1.csv");

        // set_thread_priority(acq_thread, acq_priority);
        // set_thread_priority(write_thread, write_priority);
        set_thread_priority(model_thread, model_priority);
        // set_thread_priority(log_thread, log_priority);
        // set_thread_priority(dac_thread, dac_priority);

        if (acq_thread.joinable())
            acq_thread.join();
        if (model_thread.joinable())
            model_thread.join();
        if (save_output_csv && log_thread.joinable())
            log_thread.join();
        if (output_to_dac && dac_thread.joinable())
            dac_thread.join();
        if (save_data && write_thread.joinable())
            write_thread.join();

        std::cout << "Child Process 1 (CH1) finished." << std::endl;
        exit(0);
    }

    pid2 = fork();

    if (pid2 < 0)
    {
        std::cerr << "Fork for CH2 failed!" << std::endl;
        return -1;
    }
    else if (pid2 == 0)
    {
        std::cout << "Child Process 2 (CH2) started. PID: " << getpid() << std::endl;

        int shm_fd_counters_ch2 = shm_open(SHM_COUNTERS, O_RDWR, 0666);
        shared_counters_t *shared_counters_ch2 = (shared_counters_t *)mmap(
            0, sizeof(shared_counters_t) * 2, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_counters_ch2, 0);
        if (shared_counters_ch2 == MAP_FAILED)
        {
            std::cerr << "Shared memory mapping failed in CH2!" << std::endl;
            exit(-1);
        }

        channel2.counters = &shared_counters_ch2[1];
        set_process_affinity(1);

        wait_for_barrier(shared_counters_ch2[0].ready_barrier, 2);
        std::thread acq_thread(acquire_data, std::ref(channel2), RP_CH_2);
        std::thread model_thread(model_inference, std::ref(channel2));

        std::thread log_thread, write_thread, dac_thread;

        if (save_output_csv)
            log_thread = std::thread(log_results, std::ref(channel2), "ModelOutput/output_ch2.csv");
        if (output_to_dac)
            dac_thread = std::thread(dac_output_loop, std::ref(channel2), RP_CH_2);
        if (save_data)
            write_thread = std::thread(write_data, std::ref(channel2), "DataOutput/data_ch2.csv");

        // set_thread_priority(acq_thread, acq_priority);
        // set_thread_priority(write_thread, write_priority);
        set_thread_priority(model_thread, model_priority);
        // set_thread_priority(log_thread, log_priority);
        // set_thread_priority(dac_thread, dac_priority);

        if (acq_thread.joinable())
            acq_thread.join();
        if (model_thread.joinable())
            model_thread.join();
        if (save_output_csv && log_thread.joinable())
            log_thread.join();
        if (output_to_dac && dac_thread.joinable())
            dac_thread.join();
        if (save_data && write_thread.joinable())
            write_thread.join();

        std::cout << "Child Process 2 (CH2) finished." << std::endl;
        exit(0);
    }

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    std::cout << "Both child processes finished." << std::endl;

    cleanup();
    print_channel_stats(shared_counters);
    shm_unlink(SHM_COUNTERS);

    return 0;
}
