//
// Created by kaina on 2024/7/21.
//

#ifndef AAUDIORECORDER_AAUDIO_RECORDER_H
#define AAUDIORECORDER_AAUDIO_RECORDER_H

#include <aaudio/AAudio.h>

#define ENABLE_CALLBACK
#define USE_WAV_HEADER
#define MAX_DATA_SIZE (1 * 1024 * 1024 * 1024)

class AAudioRecorder
{
private:
    aaudio_input_preset_t inputPreset;
    int32_t sampleRate;
    int32_t channelCount;
    // aaudio_channel_mask_t channelMask;
    aaudio_format_t format;
    int32_t framesPerBurst;
    int32_t numOfBursts;
    aaudio_direction_t direction;
    aaudio_sharing_mode_t sharingMode;
    aaudio_performance_mode_t performanceMode;

    bool isPlaying;
    AAudioStream *aaudioStream;
    std::string audioFile;

    static int32_t bytesPerFrame;
    static int32_t totalBytesRead;
    static std::ofstream outputFile;

#ifdef ENABLE_CALLBACK
    static aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
    static void errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error);
#endif

public:
    AAudioRecorder();
    ~AAudioRecorder();

    void startAAudioCapture();
    void stopAAudioCapture();
    void stopCapture();
};

#endif // AAUDIORECORDER_AAUDIO_RECORDER_H
