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

    bool startAAudioCapture();
    bool stopAAudioCapture();

private:
    aaudio_input_preset_t m_inputPreset;
    int32_t m_sampleRate;
    int32_t m_channelCount;
    aaudio_format_t m_format;
    int32_t m_framesPerBurst;
    int32_t m_numOfBursts;
    aaudio_direction_t m_direction;
    aaudio_sharing_mode_t m_sharingMode;
    aaudio_performance_mode_t m_performanceMode;

    bool m_isRecording;
    AAudioStream *m_aaudioStream;
    std::string m_audioFile;
#ifdef ENABLE_CALLBACK
    SharedBuffer *m_sharedBuf;
#endif

    void _stopCapture();
};
#endif // AAUDIORECORDER_AAUDIO_RECORDER_H
