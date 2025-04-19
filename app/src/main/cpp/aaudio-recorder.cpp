#include <jni.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <unistd.h>
#include "common.h"
#include "aaudio-recorder.h"
#include "wav-header.h"

void get_format_time(char *format_time);
int32_t getBytesPerSample(aaudio_format_t format);
#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);
void errorCallback([[maybe_unused]] [[maybe_unused]] AAudioStream *stream, [[maybe_unused]] void *userData, aaudio_result_t result);
#endif

AAudioRecorder::AAudioRecorder() : mInputPreset(AAUDIO_INPUT_PRESET_VOICE_RECOGNITION),
                                   mSampleRate(48000),
                                   mChannelCount(1),
                                   mFormat(AAUDIO_FORMAT_PCM_I16),
                                   mFramesPerBurst(480),
                                   mNumOfBursts(2),
                                   mDirection(AAUDIO_DIRECTION_INPUT),
                                   mSharingMode(AAUDIO_SHARING_MODE_SHARED),
                                   mPerformanceMode(AAUDIO_PERFORMANCE_MODE_LOW_LATENCY),
                                   mIsRecording(false),
                                   mAAudioStream(nullptr),
                                   mAudioFile("/data/record_48k_1ch_16bit.wav")
{
#ifdef ENABLE_CALLBACK
    mSharedBuf = new SharedBuffer(static_cast<size_t>(mSampleRate / 1000 * 40 * mChannelCount * 2));
#endif
}

AAudioRecorder::~AAudioRecorder()
{
#ifdef ENABLE_CALLBACK
    if (mSharedBuf)
    {
        delete mSharedBuf;
        mSharedBuf = nullptr;
    }
#endif
}

bool AAudioRecorder::startAAudioCapture()
{
    bool isFileCreated = false;
    AAudioStreamBuilder *builder{nullptr};
    aaudio_stream_state_t state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    char *bufWrite2File = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
    }
    AAudioStreamBuilder_setInputPreset(builder, mInputPreset);
    AAudioStreamBuilder_setSampleRate(builder, mSampleRate);
    AAudioStreamBuilder_setChannelCount(builder, mChannelCount);
    // AAudioStreamBuilder_setChannelMask(builder, m_channelMask);
    AAudioStreamBuilder_setFormat(builder, mFormat);
    AAudioStreamBuilder_setDirection(builder, mDirection);
    AAudioStreamBuilder_setPerformanceMode(builder, mPerformanceMode);
    AAudioStreamBuilder_setSharingMode(builder, mSharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, mSampleRate / 1000 * 40);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#ifdef ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, (void *)mSharedBuf);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: InputPreset:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", mInputPreset, mSampleRate,
          mChannelCount, mFormat);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &mAAudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
    }
    mFramesPerBurst = AAudioStream_getFramesPerBurst(mAAudioStream);
    AAudioStream_setBufferSizeInFrames(mAAudioStream, mNumOfBursts * mFramesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(mAAudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(mAAudioStream);
    int32_t actualDataFormat = AAudioStream_getFormat(mAAudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(mAAudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n",
          actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize, mFramesPerBurst);

    int32_t bytesPerFrame = getBytesPerSample(actualDataFormat) * actualChannelCount;
    /************** set audio file path **************/
    char audioFileArr[256] = {0};
    char formatTime[32] = {0};
    get_format_time(formatTime);
#ifdef USE_WAV_HEADER
    snprintf(audioFileArr, sizeof(audioFileArr), "/data/record_%dk_%dch_%dbit.wav", actualSampleRate / 1000,
             actualChannelCount, bytesPerFrame / actualChannelCount * 8);
//    snprintf(audioFileArr, sizeof(audioFileArr), "/data/record_%dk_%dch_%dbit_%s.wav", actualSampleRate/1000,
//             actualChannelCount, bytesPerFrame/actualChannelCount * 8, formatTime);
#else
    snprintf(audioFileArr, sizeof(audioFileArr), "/data/record_%dk_%dch_%dbit_%s.pcm", actualSampleRate / 1000,
             actualChannelCount, bytesPerFrame / actualChannelCount * 8, formatTime);
#endif
    mAudioFile = std::string(audioFileArr);
    ALOGI("Audio file path: %s\n", mAudioFile.c_str());
    std::ofstream outputFile(mAudioFile, std::ios::binary | std::ios::out);
    if (!outputFile.is_open() || outputFile.fail())
    {
        isFileCreated = false;
        ALOGE("AAudioRecorder error opening file\n");
        // goto exit_label;
    } else {
        isFileCreated = true;
    }

#ifdef USE_WAV_HEADER
    /************** write audio file header **************/
    if (isFileCreated) {
        if (!writeWAVHeader(outputFile, 0, actualSampleRate, actualChannelCount,
                            bytesPerFrame / actualChannelCount * 8))
        {
            ALOGE("writeWAVHeader failed\n");
            goto exit_label;
        }
    }
#endif

    result = AAudioStream_requestStart(mAAudioStream);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        goto exit_label;
    }
    state = AAudioStream_getState(mAAudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

#ifdef ENABLE_CALLBACK
    mSharedBuf->setBufSize(mFramesPerBurst * bytesPerFrame * 8);
#endif
    bufWrite2File = new (std::nothrow) char[mFramesPerBurst * bytesPerFrame * 2];
    if (!bufWrite2File)
    {
        ALOGE("AAudioRecorder new bufWrite2File failed\n");
        goto exit_label;
    }
    mIsRecording = true;
    while (mAAudioStream)
    {
        memset(bufWrite2File, 0, mFramesPerBurst * bytesPerFrame * 2);
#ifdef ENABLE_CALLBACK
        usleep(8 * 1000);
        bool ret = mSharedBuf->consume(bufWrite2File, mFramesPerBurst * bytesPerFrame * 2);
        if (ret && isFileCreated)
        {
            outputFile.write(bufWrite2File, mFramesPerBurst * bytesPerFrame);
        }
#else
        // block read as timeoutNanoseconds is not zero
        int32_t rst = AAudioStream_read(mAAudioStream, (void *)bufWrite2File, mFramesPerBurst, 60 * 1000 * 1000);
        if (rst >= 0)
        {
            // ALOGD("AAudio actually read frames %d, should read frames %d\n", rst, mFramesPerBurst);
            if (rst != mFramesPerBurst)
            {
                ALOGW("AAudio actually read frames %d, should read frames %d\n", rst, mFramesPerBurst);
            }
            if (isFileCreated) {
                outputFile.write(bufWrite2File, mFramesPerBurst * bytesPerFrame);
            }
        }
        else
        {
            ALOGE("AAudio read error\n");
            mIsRecording = false;
        }
#endif
        int64_t totalBytesRead = AAudioStream_getFramesRead(mAAudioStream) * bytesPerFrame;
        if (totalBytesRead >= MAX_DATA_SIZE)
        {
            ALOGE("AAudio read data size exceeds limit: %d MB, stop record\n", MAX_DATA_SIZE / (1024 * 1024));
            mIsRecording = false;
        }
        if (!mIsRecording)
        {
#ifdef USE_WAV_HEADER
            if (isFileCreated) {
                UpdateSizes(outputFile, totalBytesRead); // update RIFF chunk size and data chunk size
            }
#endif
            _stopCapture();
            goto exit_label;
        }
    }

exit_label:
    if (mAAudioStream != nullptr)
    {
        AAudioStream_close(mAAudioStream);
        mAAudioStream = nullptr;
    }
    if (isFileCreated && outputFile.is_open())
    {
        outputFile.close();
    }
    delete[] bufWrite2File;

    return true;
}

bool AAudioRecorder::stopAAudioCapture()
{
    mIsRecording = false;
    return true;
}

void AAudioRecorder::_stopCapture()
{
    int32_t xRunCount = AAudioStream_getXRunCount(mAAudioStream);
    ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
    aaudio_result_t result = AAudioStream_requestStop(mAAudioStream);
    if (result == AAUDIO_OK)
    {
        aaudio_stream_state_t currentState = AAudioStream_getState(mAAudioStream);
        aaudio_stream_state_t inputState = currentState;
        while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
        {
            result = AAudioStream_waitForStateChange(mAAudioStream, inputState, &currentState, 60 * 1000 * 1000);
            inputState = currentState;
        }
    }
    else
    {
        ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
    }

    aaudio_stream_state_t currentState = AAudioStream_getState(mAAudioStream);
    if (currentState != AAUDIO_STREAM_STATE_STOPPED)
    {
        ALOGW("AAudioStream_getState %s\n", AAudio_convertStreamStateToText(currentState));
    }
    if (mAAudioStream)
    {
        AAudioStream_close(mAAudioStream);
        mAAudioStream = nullptr;
    }
}

void get_format_time(char *format_time)
{
    time_t t = time(nullptr);
    struct tm *now = localtime(&t);
    strftime(format_time, 32, "%Y%m%d_%H.%M.%S", now);
}

int32_t getBytesPerSample(aaudio_format_t format)
{
    switch (format)
    {
    case AAUDIO_FORMAT_PCM_I16:
        return 2;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        return 3;
    case AAUDIO_FORMAT_PCM_I32:
    case AAUDIO_FORMAT_PCM_FLOAT:
        return 4;
    default:
        return 2;
    }
}

#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
    if (numFrames > 0)
    {
        int32_t channels = AAudioStream_getChannelCount(stream);
        int32_t bytesPerFrame = getBytesPerSample(AAudioStream_getFormat(stream)) * channels;
        if (!userData)
        {
            return AAUDIO_CALLBACK_RESULT_STOP;
        }
        bool ret = ((SharedBuffer *)userData)->produce((char *)audioData, numFrames * bytesPerFrame);
        if (!ret)
        {
            ALOGE("can't write to buffer, buffer is full\n");
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void errorCallback([[maybe_unused]] AAudioStream *stream, [[maybe_unused]] void *userData, aaudio_result_t result)
{
    ALOGE("AAudio errorCallback, result: %d %s\n", result, AAudio_convertResultToText(result));
}
#endif // ENABLE_CALLBACK

AAudioRecorder *AARecorder{nullptr};
extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_MainActivity_startAAudioCaptureFromJNI([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz)
{
    // TODO: implement startAAudioCaptureFromJNI()
    AARecorder = new AAudioRecorder();
    AARecorder->startAAudioCapture();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_MainActivity_stopAAudioCaptureFromJNI([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jobject thiz)
{
    // TODO: implement stopAAudioCaptureFromJNI()
    AARecorder->stopAAudioCapture();
}
