#include <jni.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <aaudio/AAudio.h>
#include "common.h"
#include "aaudio-recorder.h"
#include "wav-header.h"

int32_t AAudioRecorder::bytesPerFrame = 0;
int32_t AAudioRecorder::totalBytesRead = 0;
std::ofstream AAudioRecorder::outputFile;

void get_format_time(char *format_time)
{
    time_t t = time(nullptr);
    struct tm *now = localtime(&t);
    strftime(format_time, 32, "%Y%m%d_%H.%M.%S", now);
}

AAudioRecorder::AAudioRecorder() : inputPreset(AAUDIO_INPUT_PRESET_VOICE_RECOGNITION),
                                   sampleRate(48000),
                                   channelCount(1),
                                   // channelMask(AAUDIO_CHANNEL_MONO),
                                   format(AAUDIO_FORMAT_PCM_I16),
                                   framesPerBurst(480),
                                   numOfBursts(2),
                                   direction(AAUDIO_DIRECTION_INPUT),
                                   sharingMode(AAUDIO_SHARING_MODE_SHARED),
                                   performanceMode(AAUDIO_PERFORMANCE_MODE_LOW_LATENCY),
                                   isPlaying(false),
                                   aaudioStream(nullptr),
                                   audioFile("/data/record_48k_1ch_16bit.wav")
{
    bytesPerFrame = 0;
    totalBytesRead = 0;
}

AAudioRecorder::~AAudioRecorder() = default;

void AAudioRecorder::startAAudioCapture()
{
    AAudioStreamBuilder *builder{nullptr};
    // Use an AAudioStreamBuilder to contain requested parameters.
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        return;
    }
    AAudioStreamBuilder_setInputPreset(builder, inputPreset);
    AAudioStreamBuilder_setSampleRate(builder, sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, channelCount);
    // AAudioStreamBuilder_setChannelMask(builder, channelMask);
    AAudioStreamBuilder_setFormat(builder, format);
    AAudioStreamBuilder_setDirection(builder, direction);
    AAudioStreamBuilder_setPerformanceMode(builder, performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, sharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, sampleRate / 1000 * 40);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#ifdef ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: inputPreset:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", inputPreset, sampleRate,
          channelCount, format);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &aaudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        return;
    }
    framesPerBurst = AAudioStream_getFramesPerBurst(aaudioStream);
    AAudioStream_setBufferSizeInFrames(aaudioStream, numOfBursts * framesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(aaudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(aaudioStream);
    int32_t actualDataFormat = AAudioStream_getFormat(aaudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(aaudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n",
          actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize, framesPerBurst);

    switch (actualDataFormat)
    {
    case AAUDIO_FORMAT_PCM_FLOAT:
        bytesPerFrame = actualChannelCount * 4;
        break;
    case AAUDIO_FORMAT_PCM_I16:
        bytesPerFrame = actualChannelCount * 2;
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        bytesPerFrame = actualChannelCount * 3;
        break;
    case AAUDIO_FORMAT_PCM_I32:
        bytesPerFrame = actualChannelCount * 4;
        break;
    default:
        bytesPerFrame = actualChannelCount * 2;
        break;
    }

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
    audioFile = std::string(audioFileArr);
    ALOGI("Audio file path: %s\n", audioFile.c_str());

    /************** open output file **************/
    outputFile.open(audioFile, std::ios::binary | std::ios::out);
    if (!outputFile.is_open() || outputFile.fail())
    {
        ALOGE("AAudioRecorder error opening file\n");
        AAudioStream_close(aaudioStream);
        return;
    }

#ifdef USE_WAV_HEADER
    /************** write audio file header **************/
    int32_t numSamples = 0;
    if (!writeWAVHeader(outputFile, numSamples, actualSampleRate, actualChannelCount,
                        bytesPerFrame / actualChannelCount * 8))
    {
        ALOGE("writeWAVHeader failed\n");
        AAudioStream_close(aaudioStream);
        outputFile.close();
        return;
    }
#endif

    totalBytesRead = 0;
    result = AAudioStream_requestStart(aaudioStream);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        if (aaudioStream != nullptr)
        {
            AAudioStream_close(aaudioStream);
            aaudioStream = nullptr;
        }
        return;
    }
    aaudio_stream_state_t state = AAudioStream_getState(aaudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

    isPlaying = true;
    std::vector<char> dataBuf(actualBufferSize * bytesPerFrame);
    while (aaudioStream)
    {
#ifdef ENABLE_CALLBACK
        usleep(10 * 1000);
#else
        int32_t framesRead = AAudioStream_read(aaudioStream, (void *)dataBuf.data(), framesPerBurst, 60 * 1000 * 1000);
        if (framesRead)
        {
            // ALOGD("aaudio read, framesRead:%d, framesPerBurst:%d\n", framesRead, framesPerBurst);
            outputFile.write((char *)dataBuf.data(), framesRead * bytesPerFrame);
            totalBytesRead += framesRead * bytesPerFrame;
        }
#endif
        if (totalBytesRead >= MAX_DATA_SIZE)
        {
            ALOGE("AudioRecord data size exceeds limit: %d MB, stop record\n", MAX_DATA_SIZE / (1024 * 1024));
            isPlaying = false;
        }
        if (!isPlaying)
        {
#ifdef USE_WAV_HEADER
            UpdateSizes(outputFile, totalBytesRead); // update RIFF chunk size and data chunk size
#endif
            stopCapture();
        }
    }
}

void AAudioRecorder::stopAAudioCapture()
{
    isPlaying = false;
}

void AAudioRecorder::stopCapture()
{
    int32_t xRunCount = AAudioStream_getXRunCount(aaudioStream);
    ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
    aaudio_result_t result = AAudioStream_requestStop(aaudioStream);
    if (result == AAUDIO_OK)
    {
        aaudio_stream_state_t currentState = AAudioStream_getState(aaudioStream);
        aaudio_stream_state_t inputState = currentState;
        while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
        {
            result = AAudioStream_waitForStateChange(aaudioStream, inputState, &currentState, 60 * 1000 * 1000);
            inputState = currentState;
        }
    }
    else
    {
        ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
    }

    aaudio_stream_state_t currentState = AAudioStream_getState(aaudioStream);
    if (currentState != AAUDIO_STREAM_STATE_STOPPED)
    {
        ALOGW("AAudioStream_getState %s\n", AAudio_convertStreamStateToText(currentState));
    }
    if (aaudioStream != nullptr)
    {
        AAudioStream_close(aaudioStream);
        aaudioStream = nullptr;
    }
    if (outputFile.is_open())
        outputFile.close();
}

#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t
AAudioRecorder::dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
    // ALOGI("aaudio dataCallback, numFrames:%d, channelCount:%d, bytesPerChannel:%d\n", numFrames, channelCount,
    // bytesPerChannel);
    if (numFrames > 0)
    {
        if (outputFile.is_open())
        {
            outputFile.write(static_cast<const char *>(audioData), numFrames * bytesPerFrame);
            // ALOGD("aaudio dataCallback, numFrames:%d\n", numFrames);
        }
        else
        {
            ALOGI("aaudio dataCallback end\n");
            return AAUDIO_CALLBACK_RESULT_STOP;
        }
        totalBytesRead += numFrames * bytesPerFrame;
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AAudioRecorder::errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error)
{
    ALOGI("errorCallback\n");
}
#endif

AAudioRecorder *AARecorder{nullptr};
extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_MainActivity_startAAudioCaptureFromJNI(JNIEnv *env, jobject thiz)
{
    // TODO: implement startAAudioCaptureFromJNI()
    AARecorder = new AAudioRecorder();
    AARecorder->startAAudioCapture();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_MainActivity_stopAAudioCaptureFromJNI(JNIEnv *env, jobject thiz)
{
    // TODO: implement stopAAudioCaptureFromJNI()
    AARecorder->stopAAudioCapture();
}
