/* modelProcessing.cpp */

#include "ModelProcessing.hpp"
#include <iostream>
#include <chrono>
#include <type_traits>

#define WITH_CMSIS_NN 1
#define ARM_MATH_DSP 1
#define ARM_NN_TRUNCATE

extern bool save_output_csv;
extern bool save_output_dac;

template <typename T>
void sample_norm(T (&data)[MODEL_INPUT_DIM_0][MODEL_INPUT_DIM_1])
{
    using base_t = typename std::remove_cv<typename std::remove_reference<decltype(data[0][0])>::type>::type;

    base_t min_val = data[0][0];
    base_t max_val = data[0][0];

    for (size_t i = 1; i < MODEL_INPUT_DIM_0; ++i)
    {
        if (data[i][0] < min_val)
            min_val = data[i][0];
        if (data[i][0] > max_val)
            max_val = data[i][0];
    }

    base_t range = max_val - min_val;
    if (range == 0)
        range = 1;

    for (size_t i = 0; i < MODEL_INPUT_DIM_0; ++i)
    {
        if constexpr (std::is_floating_point<base_t>::value)
        {
            data[i][0] = static_cast<base_t>((data[i][0] - min_val) / static_cast<float>(range));
        }
        else
        {
            data[i][0] = static_cast<base_t>(((data[i][0] - min_val) * 512) / range);
        }
    }
}

void model_inference(Channel &channel)
{
    try
    {
        while (true)
        {
            if (sem_wait(&channel.model_sem) != 0)
            {
                if (errno == EINTR && stop_program.load())
                    break;
                continue;
            }

            if (stop_program.load() && channel.model_queue.empty())
                break;

            while (!channel.model_queue.empty())
            {
                std::shared_ptr<data_part_t> part = channel.model_queue.front();
                channel.model_queue.pop();

                model_result_t result;
                auto start = std::chrono::high_resolution_clock::now();
                cnn(part->data, result.output);
                auto end = std::chrono::high_resolution_clock::now();
                result.computation_time = std::chrono::duration<double, std::milli>(end - start).count();

                if (save_output_csv)
                {
                    channel.result_buffer_csv.push_back(result);
                    sem_post(&channel.result_sem_csv);
                }

                if (save_output_dac)
                {
                    channel.result_buffer_dac.push_back(result);
                    sem_post(&channel.result_sem_dac);
                }

                channel.model_count.fetch_add(1, std::memory_order_relaxed);
            }

            if (channel.acquisition_done && channel.model_queue.empty())
                break;
        }

        channel.processing_done = true;
        if (save_output_csv)
            sem_post(&channel.result_sem_csv);
        if (save_output_dac)
            sem_post(&channel.result_sem_dac);

        std::cout << "Model inference thread on channel " << static_cast<int>(channel.channel_id) + 1 << " exiting..." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in model_inference: " << e.what() << std::endl;
    }
}


void model_inference_mod(Channel &channel)
{
    try
    {
        while (true)
        {
            if (sem_wait(&channel.model_sem) != 0)
            {
                if (errno == EINTR && stop_program.load())
                    break;
                continue;
            }

            if (stop_program.load() && channel.model_queue.empty())
                break;

            while (!channel.model_queue.empty())
            {
                std::shared_ptr<data_part_t> part = channel.model_queue.front();
                channel.model_queue.pop();

                sample_norm(part->data); 

                model_result_t result;
                auto start = std::chrono::high_resolution_clock::now();
                cnn(part->data, result.output);
                auto end = std::chrono::high_resolution_clock::now();
                result.computation_time = std::chrono::duration<double, std::milli>(end - start).count();

                if (save_output_csv)
                {
                    channel.result_buffer_csv.push_back(result);
                    sem_post(&channel.result_sem_csv);
                }

                if (save_output_dac)
                {
                    channel.result_buffer_dac.push_back(result);
                    sem_post(&channel.result_sem_dac);
                }

                channel.model_count.fetch_add(1, std::memory_order_relaxed);
            }

            if (channel.acquisition_done && channel.model_queue.empty())
                break;
        }

        channel.processing_done = true;
        if (save_output_csv)
            sem_post(&channel.result_sem_csv);
        if (save_output_dac)
            sem_post(&channel.result_sem_dac);

        std::cout << "Model inference mod thread on channel " << static_cast<int>(channel.channel_id) + 1 << " exiting..." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in model_inference_mod: " << e.what() << std::endl;
    }
}
