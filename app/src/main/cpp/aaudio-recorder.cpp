#include <jni.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <aaudio/AAudio.h>
#include "common.h"

aaudio_input_preset_t inputPreset = AAUDIO_INPUT_PRESET_VOICE_RECOGNITION;
int32_t sampleRate = 48000;
int32_t channelCount = 1;
// aaudio_channel_mask_t channelMask = AAUDIO_CHANNEL_MONO;
aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;
aaudio_direction_t direction = AAUDIO_DIRECTION_INPUT;
aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
int32_t capacityInFrames = sampleRate / 1000 * 40;
int32_t framesPerBurst = sampleRate / 1000 * 10;
int32_t bytesPerChannel = 2;
int32_t numOfBursts = 2;
bool isStart = false;

AAudioStreamBuilder *builder = nullptr;
AAudioStream *aaudioStream = nullptr;
std::ofstream outputFile;
std::string audioFile = "/data/record_48k_1ch_16bit.raw";
// std::string audioFile = "/data/data/com.example.aaudiorecorder/files/record_48k_1ch_16bit.raw";

#if ENABLE_CALLBACK
aaudio_data_callback_result_t dataCallback(AAudioStream *stream __unused, void *userData __unused, void *audioData,
                                           int32_t numFrames)
{
    // ALOGI("aaudio dataCallback, numFrames:%d, channelCount:%d, bytesPerChannel:%d\n", numFrames, channelCount,
    // bytesPerChannel);
    if (numFrames > 0) {
        if (outputFile.is_open()) {
            outputFile.write(static_cast<const char *>(audioData), numFrames * channelCount * bytesPerChannel);
            // ALOGD("aaudio dataCallback, numFrames:%d\n", numFrames);
        } else {
            ALOGI("aaudio dataCallback end\n");
            return AAUDIO_CALLBACK_RESULT_STOP;
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void errorCallback(AAudioStream *stream __unused, void *userData __unused, aaudio_result_t error __unused)
{
    ALOGI("errorCallback\n");
}
#endif

void stopCapture();
bool stopAAudioCapture();
bool startAAudioCapture() {
    ALOGI("start AAudioCapture, isStart: %d\n", isStart);
    if (isStart) {
        ALOGI("in starting status, needn't start again\n");
        return false;
    }
    if (aaudioStream && AAudioStream_getState(aaudioStream) != AAUDIO_STREAM_STATE_CLOSED) {
        ALOGI("stream not in stopped status, try again later\n"); // avoid start again during closing
        return false;
    }
    isStart = true;

    // prepare data
    outputFile.open(audioFile, std::ios::out | std::ios::binary);
    if (!outputFile.is_open()) {
        ALOGE("AAudioRecorder error opening file\n");
        isStart = false;
        return false;
    }

    // Use an AAudioStreamBuilder to contain requested parameters.
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        isStart = false;
        return false;
    }
    AAudioStreamBuilder_setInputPreset(builder, inputPreset);
    AAudioStreamBuilder_setSampleRate(builder, sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, channelCount);
    // AAudioStreamBuilder_setChannelMask(builder, channelMask);
    AAudioStreamBuilder_setFormat(builder, format);
    AAudioStreamBuilder_setDirection(builder, direction);
    AAudioStreamBuilder_setPerformanceMode(builder, performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, sharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, capacityInFrames);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#if ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: inputPreset:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", inputPreset, sampleRate,
          channelCount, format);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &aaudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK) {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        isStart = false;
        return false;
    }
    framesPerBurst = AAudioStream_getFramesPerBurst(aaudioStream);
    AAudioStream_setBufferSizeInFrames(aaudioStream, numOfBursts * framesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(aaudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(aaudioStream);
    int32_t actualDataFormat = AAudioStream_getFormat(aaudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(aaudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n", actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize,
          framesPerBurst);

    switch (actualDataFormat)
    {
        case AAUDIO_FORMAT_PCM_FLOAT:
            bytesPerChannel = 4;
            break;
        case AAUDIO_FORMAT_PCM_I16:
            bytesPerChannel = 2;
            break;
        case AAUDIO_FORMAT_PCM_I24_PACKED:
            bytesPerChannel = 3;
            break;
        case AAUDIO_FORMAT_PCM_I32:
            bytesPerChannel = 4;
            break;
        default:
            bytesPerChannel = 2;
            break;
    }

    result = AAudioStream_requestStart(aaudioStream);
    if (result != AAUDIO_OK) {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        if (aaudioStream != nullptr) {
            AAudioStream_close(aaudioStream);
            aaudioStream = nullptr;
        }
        isStart = false;
        return false;
    }
    aaudio_stream_state_t state = AAudioStream_getState(aaudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

    std::vector<char> dataBuf(actualBufferSize * actualChannelCount * bytesPerChannel);
    while (aaudioStream) {
#ifdef ENABLE_CALLBACK
        usleep(10 * 1000);
#else
        int32_t framesRead = AAudioStream_read(aaudioStream, (void *)dataBuf.data(), framesPerBurst, 60 * 1000 * 1000);
        if (framesRead) {
            // ALOGD("aaudio read, framesRead:%d, framesPerBurst:%d\n", framesRead, framesPerBurst);
            outputFile.write((char *)dataBuf.data(), framesRead * actualChannelCount * bytesPerChannel);
        }
#endif
        if (!isStart)
            stopCapture();
    }
    return true;
}

bool stopAAudioCapture() {
    ALOGI("stop AAudioCapture, isStart: %d\n", isStart);
    if (isStart) {
		isStart = false;
    }
    return true;
}

void stopCapture() {
    int32_t xRunCount = AAudioStream_getXRunCount(aaudioStream);
    ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
    aaudio_result_t result = AAudioStream_requestStop(aaudioStream);
    if (result == AAUDIO_OK) {
        aaudio_stream_state_t currentState = AAudioStream_getState(aaudioStream);
        aaudio_stream_state_t inputState = currentState;
        while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
        {
            result = AAudioStream_waitForStateChange(aaudioStream, inputState, &currentState, 60 * 1000 * 1000);
            inputState = currentState;
        }
    } else {
        ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
    }

    aaudio_stream_state_t currentState = AAudioStream_getState(aaudioStream);
    if (currentState != AAUDIO_STREAM_STATE_STOPPED) {
        ALOGW("AAudioStream_getState %s\n", AAudio_convertStreamStateToText(currentState));
    }
    if (aaudioStream != nullptr) {
        AAudioStream_close(aaudioStream);
        aaudioStream = nullptr;
    }
    if (outputFile.is_open())
        outputFile.close();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_MainActivity_startAAudioCaptureFromJNI(JNIEnv *env __unused, jobject thiz __unused) {
    // TODO: implement startAAudioCaptureFromJNI()
    startAAudioCapture();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_MainActivity_stopAAudioCaptureFromJNI(JNIEnv *env __unused, jobject thiz __unused) {
    // TODO: implement stopAAudioCaptureFromJNI()
    stopAAudioCapture();
}
