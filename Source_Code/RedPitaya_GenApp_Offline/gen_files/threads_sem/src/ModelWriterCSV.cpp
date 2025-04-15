/*ModelWriterCSV.cpp*/

#include "ModelWriterCSV.hpp"
#include <iostream>
#include <cstdio>
#include <type_traits>
#include <mutex>

template <typename T>
void write_output(FILE *file, int index, const T &value, double time_ms)
{
    if constexpr (std::is_integral<T>::value)
    {
        fprintf(file, "%d,%d,%.6f\n", index, static_cast<int>(value), time_ms);
    }
    else if constexpr (std::is_floating_point<T>::value)
    {
        fprintf(file, "%d,%.6f,%.6f\n", index, value, time_ms);
    }
    else
    {
        fprintf(file, "%d,%d,%.6f\n", index, static_cast<int>(value), time_ms);
    }
}

void log_results_csv(Channel &channel, const std::string &filename)
{
    try
    {
        FILE *output_file = fopen(filename.c_str(), "w");
        if (!output_file)
        {
            std::cerr << "Error opening output file: " << filename << "\n";
            return;
        }

        int output_index = 1;

        while (true)
        {
            if (sem_wait(&channel.result_sem_csv) != 0)
            {
                if (errno == EINTR && stop_program.load())
                    break;
                continue;
            }

            if (stop_program.load() && channel.result_buffer_csv.empty())
                break;

            while (!channel.result_buffer_csv.empty())
            {
                const model_result_t &result = channel.result_buffer_csv.front();
                write_output(output_file, output_index++, result.output[0], result.computation_time);
                fflush(output_file);
                channel.result_buffer_csv.pop_front();
                channel.log_count_csv.fetch_add(1, std::memory_order_relaxed);
            }

            if (channel.processing_done && channel.result_buffer_csv.empty())
                break;
        }

        fclose(output_file);
        std::cout << "Logging inference results on CSV thread on channel " << static_cast<int>(channel.channel_id) + 1 << " exiting..." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in log_results_csv for channel " << static_cast<int>(channel.channel_id) + 1 << ": " << e.what() << std::endl;
    }
}
