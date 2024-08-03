//
// Created by kaina on 2024/7/21.
//

#ifndef AAUDIORECORDER_AAUDIO_RECORDER_H
#define AAUDIORECORDER_AAUDIO_RECORDER_H

#include <aaudio/AAudio.h>
#include "aaudio-buffer.h"

#define ENABLE_CALLBACK
#define USE_WAV_HEADER
#define MAX_DATA_SIZE (1 * 1024 * 1024 * 1024)

class AAudioRecorder
{
public:
    AAudioRecorder();
    ~AAudioRecorder();

    void startAAudioCapture();
    void stopAAudioCapture();

private:
    aaudio_input_preset_t m_inputPreset;
    int32_t m_sampleRate;
    int32_t m_channelCount;
    // aaudio_channel_mask_t m_channelMask;
    aaudio_format_t m_format;
    int32_t m_framesPerBurst;
    int32_t m_numOfBursts;
    aaudio_direction_t m_direction;
    aaudio_sharing_mode_t m_sharingMode;
    aaudio_performance_mode_t m_performanceMode;

    bool m_isPlaying;
    AAudioStream *m_aaudioStream;
    std::string m_audioFile;

    void _stopCapture();
    static int32_t _getBytesPerSample(aaudio_format_t format);

#ifdef ENABLE_CALLBACK
    SharedBuffer *m_sharedBuf;
    static aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
    static void errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error);
#endif
};
#endif // AAUDIORECORDER_AAUDIO_RECORDER_H
