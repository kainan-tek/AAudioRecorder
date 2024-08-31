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
    aaudio_input_preset_t mInputPreset;
    int32_t mSampleRate;
    int32_t mChannelCount;
    aaudio_format_t mFormat;
    int32_t mFramesPerBurst;
    int32_t mNumOfBursts;
    aaudio_direction_t mDirection;
    aaudio_sharing_mode_t mSharingMode;
    aaudio_performance_mode_t mPerformanceMode;

    bool mIsRecording;
    AAudioStream *mAAudioStream;
    std::string mAudioFile;
#ifdef ENABLE_CALLBACK
    SharedBuffer *mSharedBuf;
#endif

    void _stopCapture();
};
#endif // AAUDIORECORDER_AAUDIO_RECORDER_H
