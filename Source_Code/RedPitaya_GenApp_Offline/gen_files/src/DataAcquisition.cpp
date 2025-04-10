/*DataAcquisition.cpp*/

#include "DataAcquisition.hpp"
#include "SystemUtils.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// Function to initialize data acquisition
void initialize_acq()
{
    rp_AcqReset();
    // Configure Red Pitaya for split trigger mode
    if (rp_AcqSetSplitTrigger(true) != RP_OK)
    {
        std::cerr << "rp_AcqSetSplitTrigger failed!" << std::endl;
    }
    if (rp_AcqSetSplitTriggerPass(true) != RP_OK)
    {
        std::cerr << "rp_AcqSetSplitTriggerPass failed!" << std::endl;
    }

    uint32_t g_adc_axi_start, g_adc_axi_size;
    if (rp_AcqAxiGetMemoryRegion(&g_adc_axi_start, &g_adc_axi_size) != RP_OK)
    {
        std::cerr << "rp_AcqAxiGetMemoryRegion failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Reserved memory Start 0x" << std::hex << g_adc_axi_start << " Size 0x" << std::hex << g_adc_axi_size << std::endl;
    std::cout << std::dec;

    // Set decimation factor for both channels
    if (rp_AcqAxiSetDecimationFactorCh(RP_CH_1, DECIMATION) != RP_OK)
    {
        std::cerr << "rp_AcqAxiSetDecimationFactor failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqAxiSetDecimationFactorCh(RP_CH_2, DECIMATION) != RP_OK)
    {
        std::cerr << "rp_AcqAxiSetDecimationFactor failed!" << std::endl;
        exit(-1);
    }

    // Get and print sampling rate
    float sampling_rate;
    if (rp_AcqGetSamplingRateHz(&sampling_rate) == RP_OK)
    {
        printf("Current Sampling Rate: %.2f Hz\n", sampling_rate);
    }
    else
    {
        fprintf(stderr, "Failed to get sampling rate\n");
    }

    // Set trigger delay for both channels
    if (rp_AcqAxiSetTriggerDelay(RP_CH_1, 0) != RP_OK)
    {
        std::cerr << "rp_AcqAxiSetTriggerDelay channel 1 failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqAxiSetTriggerDelay(RP_CH_2, 0) != RP_OK)
    {
        std::cerr << "rp_AcqAxiSetTriggerDelay channel 2 failed!" << std::endl;
        exit(-1);
    }

    // Set buffer samples for both channels
    if (rp_AcqAxiSetBufferSamples(RP_CH_1, g_adc_axi_start, DATA_SIZE) != RP_OK)
    {
        std::cerr << "rp_AcqAxiSetBuffer RP_CH_1 failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqAxiSetBufferSamples(RP_CH_2, g_adc_axi_start + (g_adc_axi_size / 2), DATA_SIZE) != RP_OK)
    {
        std::cerr << "rp_AcqAxiSetBuffer RP_CH_2 failed!" << std::endl;
        exit(-1);
    }

    // Enable acquisition on both channels
    if (rp_AcqAxiEnable(RP_CH_1, true) != RP_OK)
    {
        std::cerr << "rp_AcqAxiEnable RP_CH_1 failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqAxiEnable(RP_CH_2, true) != RP_OK)
    {
        std::cerr << "rp_AcqAxiEnable RP_CH_2 failed!" << std::endl;
        exit(-1);
    }

    // Set trigger levels for both channels
    if (rp_AcqSetTriggerLevel(RP_T_CH_1, 0) != RP_OK)
    {
        std::cerr << "rp_AcqSetTriggerLevel RP_T_CH_1 failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqSetTriggerLevel(RP_T_CH_2, 0) != RP_OK)
    {
        std::cerr << "rp_AcqSetTriggerLevel RP_T_CH_2 failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqSetTriggerSrcCh(RP_CH_1, RP_TRIG_SRC_CHA_PE) != RP_OK)
    {
        std::cerr << "rp_AcqSetTriggerSrcCh RP_CH_1 failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqSetTriggerSrcCh(RP_CH_2, RP_TRIG_SRC_CHB_PE) != RP_OK)
    {
        std::cerr << "rp_AcqSetTriggerSrcCh RP_CH_2 failed!" << std::endl;
        exit(-1);
    }

    // Start acquisition on both channels
    if (rp_AcqStartCh(RP_CH_1) != RP_OK)
    {
        std::cerr << "rp_AcqStart failed!" << std::endl;
        exit(-1);
    }
    if (rp_AcqStartCh(RP_CH_2) != RP_OK)
    {
        std::cerr << "rp_AcqStart failed!" << std::endl;
        exit(-1);
    }
}

void acquire_data(Channel &channel, rp_channel_t rp_channel)
{
    try
    {
        std::cout << "Waiting for trigger on channel " << rp_channel + 1 << "..." << std::endl;

        while (!channel.channel_triggered && !stop_acquisition.load())
        {
            if (rp_AcqGetTriggerStateCh(rp_channel, &channel.state) != RP_OK)
            {
                std::cerr << "rp_AcqGetTriggerStateCh failed on channel " << rp_channel + 1 << std::endl;
                exit(-1);
            }

            if (channel.state == RP_TRIG_STATE_TRIGGERED)
            {
                channel.channel_triggered = true;
                std::cout << "Trigger detected on channel " << rp_channel + 1 << "!" << std::endl;
                channel.trigger_time_point = std::chrono::steady_clock::now();
                channel.counters->trigger_time_ns.store(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        channel.trigger_time_point.time_since_epoch())
                        .count());
            }
        }

        if (!channel.channel_triggered)
        {
            std::cerr << "INFO: Acquisition stopped before trigger detected on channel " << rp_channel + 1 << "." << std::endl;
            stop_acquisition.store(true);
            exit(-1);
        }

        std::cout << "Starting data acquisition on channel " << rp_channel + 1 << std::endl;

        uint32_t pw = 0;
        constexpr uint32_t samples_per_chunk = MODEL_INPUT_DIM_0;
        uint32_t chunk_size = samples_per_chunk;

        if (rp_AcqAxiGetWritePointerAtTrig(rp_channel, &pw) != RP_OK)
        {
            std::cerr << "Error getting write pointer at trigger for channel " << rp_channel + 1 << std::endl;
            exit(-1);
        }

        uint32_t pos = pw;

        while (!stop_acquisition.load())
        {
            if (is_disk_space_below_threshold("/", DISK_SPACE_THRESHOLD))
            {
                std::cerr << "ERR: Disk space below threshold. Stopping acquisition." << std::endl;
                stop_acquisition.store(true);
                break;
            }

            uint32_t pwrite = 0;
            if (rp_AcqAxiGetWritePointer(rp_channel, &pwrite) == RP_OK)
            {
                int64_t distance = (pwrite >= pos) ? (pwrite - pos) : (DATA_SIZE - pos + pwrite);

                if (distance < 0)
                {
                    std::cerr << "ERR: Negative distance calculated on channel " << rp_channel + 1 << std::endl;
                    continue;
                }

                if (distance >= DATA_SIZE)
                {
                    std::cerr << "ERR: Overrun detected on channel " << rp_channel + 1 << " at: " << channel.counters->acquire_count.load() << std::endl;

                    {
                        std::lock_guard<std::mutex> lock(channel.mtx);
                        stop_acquisition.store(true);
                    }
                    channel.cond_write_csv.notify_all();
                    channel.cond_model.notify_all();
                    return;
                }

                if (distance >= samples_per_chunk)
                {
                    int16_t buffer_raw[samples_per_chunk];
                    if (rp_AcqAxiGetDataRaw(rp_channel, pos, &chunk_size, buffer_raw) != RP_OK)
                    {
                        std::cerr << "rp_AcqAxiGetDataRaw failed on channel " << rp_channel + 1 << std::endl;
                        continue;
                    }

                    auto part = std::make_shared<data_part_t>();
                    convert_raw_data(buffer_raw, part->data, samples_per_chunk);

                    pos += samples_per_chunk;
                    if (pos >= DATA_SIZE)
                        pos -= DATA_SIZE;

                    {
                        std::lock_guard<std::mutex> lock(channel.mtx);
                        channel.data_queue_csv.push(part);
                        channel.model_queue_csv.push(part);
                        channel.model_queue_dac.push(part);
                        channel.cond_write_csv.notify_all();
                        channel.cond_model.notify_all();
                    }

                    channel.counters->acquire_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }

        channel.end_time_point = std::chrono::steady_clock::now();
        channel.counters->end_time_ns.store(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                channel.end_time_point.time_since_epoch())
                .count());

        {
            std::lock_guard<std::mutex> lock(channel.mtx);
            channel.acquisition_done = true;
            channel.cond_write_csv.notify_all();
            channel.cond_model.notify_all();
        }

        std::cout << "Acquisition thread on channel " << rp_channel + 1 << " exiting..." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in acquire_data for channel " << rp_channel + 1 << ": " << e.what() << std::endl;
    }
}

// Function to clean up resources used by Red Pitaya
void cleanup()
{
    std::cout << "\nReleasing resources\n";
    rp_AcqStopCh(RP_CH_1); // Stop data acquisition for CH1
    rp_AcqStopCh(RP_CH_2); // Stop data acquisition for CH2
    rp_AcqAxiEnable(RP_CH_1, false);
    rp_AcqAxiEnable(RP_CH_2, false);
    rp_Release(); // Release Red Pitaya resources
    std::cout << "Cleanup done." << std::endl;
}
