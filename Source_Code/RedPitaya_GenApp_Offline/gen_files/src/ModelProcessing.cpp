/* modelProcessing.cpp */

#include "ModelProcessing.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <type_traits>

#define WITH_CMSIS_NN 1
#define ARM_MATH_DSP 1
#define ARM_NN_TRUNCATE

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
            std::shared_ptr<data_part_t> part;
            {
                std::unique_lock<std::mutex> lock(channel.mtx);
                channel.cond_model.wait(lock, [&]
                                        { return !channel.model_queue_csv.empty() || channel.acquisition_done || stop_program.load(); });

                if (stop_program.load() && channel.acquisition_done && channel.model_queue_csv.empty())
                    break;

                if (channel.model_queue_csv.empty())
                    continue;

                part = channel.model_queue_csv.front();
                channel.model_queue_csv.pop();
            }

            model_result_t result;
            auto start = std::chrono::high_resolution_clock::now();
            cnn(part->data, result.output);
            auto end = std::chrono::high_resolution_clock::now();
            result.computation_time = std::chrono::duration<double, std::milli>(end - start).count();

            {
                std::lock_guard<std::mutex> lock(channel.mtx);
                channel.result_buffer_csv.push_back(result);
                channel.result_buffer_dac.push_back(result);
                channel.counters->model_count.fetch_add(1, std::memory_order_relaxed);
                channel.cond_log_csv.notify_all();
                channel.cond_log_dac.notify_all();
            }
        }

        {
            std::lock_guard<std::mutex> lock(channel.mtx);
            channel.processing_done = true;
            channel.cond_log_csv.notify_all();
            channel.cond_log_dac.notify_all();
        }

        std::cout << "model calculation thread exiting..." << std::endl;
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
            std::shared_ptr<data_part_t> part;
            {
                std::unique_lock<std::mutex> lock(channel.mtx);
                channel.cond_model.wait(lock, [&]
                                        { return !channel.model_queue_dac.empty() || channel.acquisition_done || stop_program.load(); });

                if (stop_program.load() && channel.acquisition_done && channel.model_queue_dac.empty())
                    break;

                if (channel.model_queue_dac.empty())
                    continue;

                part = channel.model_queue_dac.front();
                channel.model_queue_dac.pop();
            }

            sample_norm(part->data);

            model_result_t result;
            auto start = std::chrono::high_resolution_clock::now();
            cnn(part->data, result.output);
            auto end = std::chrono::high_resolution_clock::now();
            result.computation_time = std::chrono::duration<double, std::milli>(end - start).count();

            {
                std::lock_guard<std::mutex> lock(channel.mtx);
                channel.result_buffer_csv.push_back(result);
                channel.result_buffer_dac.push_back(result);
                channel.counters->model_count.fetch_add(1, std::memory_order_relaxed);
                channel.cond_log_csv.notify_all();
                channel.cond_log_dac.notify_all();
            }
        }

        {
            std::lock_guard<std::mutex> lock(channel.mtx);
            channel.processing_done = true;
            channel.cond_log_csv.notify_all();
            channel.cond_log_dac.notify_all();
        }

        std::cout << "model mod thread exiting..." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in model_inference_mod: " << e.what() << std::endl;
    }
}
