/*DAC.cpp*/

#include "DAC.hpp"
#include "rp.h"
#include <cmath>
#include <iostream>
#include <type_traits>

void initialize_DAC()
{
    rp_GenReset();
    rp_GenWaveform(RP_CH_1, RP_WAVEFORM_DC);
    rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
    rp_GenOutEnable(RP_CH_1);
    rp_GenOutEnable(RP_CH_2);
    rp_GenTriggerOnly(RP_CH_1);
    rp_GenTriggerOnly(RP_CH_2);
}

template<typename T>
float OutputToVoltage(T value)
{
    if constexpr (std::is_same_v<T, int16_t>) {
        return static_cast<float>(value) / 8192.0f;
    } else if constexpr (std::is_same_v<T, int8_t>) {
        return static_cast<float>(value) / 128.0f;
    } else if constexpr (std::is_same_v<T, float>) {
        return value;
    } else {
        return static_cast<float>(value);
    }
}

void dac_output_loop(Channel &channel, rp_channel_t rp_channel)
{
    try
    {
        while (true)
        {
            model_result_t result;

            {
                std::unique_lock<std::mutex> lock(channel.mtx);
                channel.cond_log_dac.wait(lock, [&]
                {
                    return !channel.result_buffer_dac.empty() || channel.processing_done || stop_program.load();
                });

                if (stop_program.load() && channel.result_buffer_dac.empty())
                    break;

                if (channel.result_buffer_dac.empty())
                    continue;

                result = channel.result_buffer_dac.front();
                channel.result_buffer_dac.pop_front();
            }

            float voltage = OutputToVoltage(result.output[0]);

            // Optional: Clamp voltage to DAC safe range [-1.0, 1.0]
            voltage = std::clamp(voltage, -1.0f, 1.0f);

            rp_GenAmp(rp_channel, voltage);
            channel.counters->log_count_dac.fetch_add(1, std::memory_order_relaxed);
        }

        std::cout << "DAC output thread done for channel CH" << static_cast<int>(rp_channel) + 1 << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in dac_output_loop: " << e.what() << std::endl;
    }
}

