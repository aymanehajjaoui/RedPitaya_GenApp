/* DataWriter.cpp */

#include "DataWriterDAC.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <type_traits>

void write_data_dac(Channel &channel, rp_channel_t rp_channel)
{
    try
    {
        while (true)
        {
            if (sem_wait(&channel.data_sem_dac) != 0)
            {
                if (errno == EINTR && stop_program.load())
                    break; // Handle interruption or stop signal
                continue;
            }

            // Exit if the program is stopping and the queue is empty
            if (stop_program.load() && channel.data_queue_dac.empty())
                break;

            while (!channel.data_queue_dac.empty())
            {
                std::shared_ptr<data_part_t> part = channel.data_queue_dac.front();
                channel.data_queue_dac.pop();

                for (size_t k = 0; k < MODEL_INPUT_DIM_0; k++)
                {
                    float voltage = OutputToVoltage(part->data[k][0]);
                    voltage = std::clamp(voltage, -1.0f, 1.0f);
                    rp_GenAmp(rp_channel, voltage);
                }

                channel.counters->write_count_dac.fetch_add(1, std::memory_order_relaxed);
            }

            // Final break condition
            if (channel.acquisition_done && channel.data_queue_dac.empty())
                break;
        }

        std::cout << "Data writing on DAC thread on channel " << static_cast<int>(channel.channel_id) + 1 << " exiting..." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in write_data_dac for channel " << static_cast<int>(channel.channel_id) + 1 << ": " << e.what() << std::endl;
    }
}

