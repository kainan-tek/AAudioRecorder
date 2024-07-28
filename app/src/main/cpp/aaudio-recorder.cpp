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

int32_t AAudioRecorder::ms_bytesPerFrame = 0;
int32_t AAudioRecorder::ms_totalBytesRead = 0;
std::ofstream AAudioRecorder::ms_outputFile;

void get_format_time(char *format_time)
{
    time_t t = time(nullptr);
    struct tm *now = localtime(&t);
    strftime(format_time, 32, "%Y%m%d_%H.%M.%S", now);
}

AAudioRecorder::AAudioRecorder() : m_inputPreset(AAUDIO_INPUT_PRESET_VOICE_RECOGNITION),
                                   m_sampleRate(48000),
                                   m_channelCount(1),
                                   // m_channelMask(AAUDIO_CHANNEL_MONO),
                                   m_format(AAUDIO_FORMAT_PCM_I16),
                                   m_framesPerBurst(480),
                                   m_numOfBursts(2),
                                   m_direction(AAUDIO_DIRECTION_INPUT),
                                   m_sharingMode(AAUDIO_SHARING_MODE_SHARED),
                                   m_performanceMode(AAUDIO_PERFORMANCE_MODE_LOW_LATENCY),
                                   m_isPlaying(false),
                                   m_aaudioStream(nullptr),
                                   m_audioFile("/data/record_48k_1ch_16bit.wav")
{
    ms_bytesPerFrame = 0;
    ms_totalBytesRead = 0;
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
    AAudioStreamBuilder_setInputPreset(builder, m_inputPreset);
    AAudioStreamBuilder_setSampleRate(builder, m_sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, m_channelCount);
    // AAudioStreamBuilder_setChannelMask(builder, m_channelMask);
    AAudioStreamBuilder_setFormat(builder, m_format);
    AAudioStreamBuilder_setDirection(builder, m_direction);
    AAudioStreamBuilder_setPerformanceMode(builder, m_performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, m_sharingMode);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, m_sampleRate / 1000 * 40);
    // AAudioStreamBuilder_setDeviceId(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setFramesPerDataCallback(builder, AAUDIO_UNSPECIFIED);
    // AAudioStreamBuilder_setAllowedCapturePolicy(builder, AAUDIO_ALLOW_CAPTURE_BY_ALL);
    // AAudioStreamBuilder_setPrivacySensitive(builder, false);
#ifdef ENABLE_CALLBACK
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);
#endif
    ALOGI("set AAudio params: InputPreset:%d, SampleRate:%d, ChannelCount:%d, Format:%d\n", m_inputPreset, m_sampleRate,
          m_channelCount, m_format);

    // Open an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(builder, &m_aaudioStream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStreamBuilder_openStream() returned %d %s\n", result, AAudio_convertResultToText(result));
        return;
    }
    m_framesPerBurst = AAudioStream_getFramesPerBurst(m_aaudioStream);
    AAudioStream_setBufferSizeInFrames(m_aaudioStream, m_numOfBursts * m_framesPerBurst);

    int32_t actualSampleRate = AAudioStream_getSampleRate(m_aaudioStream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(m_aaudioStream);
    int32_t actualDataFormat = AAudioStream_getFormat(m_aaudioStream);
    int32_t actualBufferSize = AAudioStream_getBufferSizeInFrames(m_aaudioStream);
    ALOGI("get AAudio params: actualSampleRate:%d, actualChannelCount:%d, actualDataFormat:%d, actualBufferSize:%d, "
          "framesPerBurst:%d\n",
          actualSampleRate, actualChannelCount, actualDataFormat, actualBufferSize, m_framesPerBurst);

    switch (actualDataFormat)
    {
    case AAUDIO_FORMAT_PCM_FLOAT:
        ms_bytesPerFrame = actualChannelCount * 4;
        break;
    case AAUDIO_FORMAT_PCM_I16:
        ms_bytesPerFrame = actualChannelCount * 2;
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        ms_bytesPerFrame = actualChannelCount * 3;
        break;
    case AAUDIO_FORMAT_PCM_I32:
        ms_bytesPerFrame = actualChannelCount * 4;
        break;
    default:
        ms_bytesPerFrame = actualChannelCount * 2;
        break;
    }

    /************** set audio file path **************/
    char audioFileArr[256] = {0};
    char formatTime[32] = {0};
    get_format_time(formatTime);
#ifdef USE_WAV_HEADER
    snprintf(audioFileArr, sizeof(audioFileArr), "/data/record_%dk_%dch_%dbit.wav", actualSampleRate / 1000,
             actualChannelCount, ms_bytesPerFrame / actualChannelCount * 8);
//    snprintf(audioFileArr, sizeof(audioFileArr), "/data/record_%dk_%dch_%dbit_%s.wav", actualSampleRate/1000,
//             actualChannelCount, ms_bytesPerFrame/actualChannelCount * 8, formatTime);
#else
    snprintf(audioFileArr, sizeof(audioFileArr), "/data/record_%dk_%dch_%dbit_%s.pcm", actualSampleRate / 1000,
             actualChannelCount, ms_bytesPerFrame / actualChannelCount * 8, formatTime);
#endif
    m_audioFile = std::string(audioFileArr);
    ALOGI("Audio file path: %s\n", m_audioFile.c_str());

    /************** open output file **************/
    ms_outputFile.open(m_audioFile, std::ios::binary | std::ios::out);
    if (!ms_outputFile.is_open() || ms_outputFile.fail())
    {
        ALOGE("AAudioRecorder error opening file\n");
        AAudioStream_close(m_aaudioStream);
        return;
    }

#ifdef USE_WAV_HEADER
    /************** write audio file header **************/
    int32_t numSamples = 0;
    if (!writeWAVHeader(ms_outputFile, numSamples, actualSampleRate, actualChannelCount,
                        ms_bytesPerFrame / actualChannelCount * 8))
    {
        ALOGE("writeWAVHeader failed\n");
        AAudioStream_close(m_aaudioStream);
        ms_outputFile.close();
        return;
    }
#endif

    ms_totalBytesRead = 0;
    result = AAudioStream_requestStart(m_aaudioStream);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        if (m_aaudioStream != nullptr)
        {
            AAudioStream_close(m_aaudioStream);
            m_aaudioStream = nullptr;
        }
        return;
    }
    aaudio_stream_state_t state = AAudioStream_getState(m_aaudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

    m_isPlaying = true;
    std::vector<char> dataBuf(actualBufferSize * ms_bytesPerFrame);
    while (m_aaudioStream)
    {
#ifdef ENABLE_CALLBACK
        usleep(10 * 1000);
#else
        int32_t framesRead = AAudioStream_read(m_aaudioStream, (void *)dataBuf.data(), m_framesPerBurst, 60 * 1000 * 1000);
        if (framesRead)
        {
            // ALOGD("aaudio read, framesRead:%d, framesPerBurst:%d\n", framesRead, m_framesPerBurst);
            ms_outputFile.write((char *)dataBuf.data(), framesRead * ms_bytesPerFrame);
            ms_totalBytesRead += framesRead * ms_bytesPerFrame;
        }
#endif
        if (ms_totalBytesRead >= MAX_DATA_SIZE)
        {
            ALOGE("AudioRecord data size exceeds limit: %d MB, stop record\n", MAX_DATA_SIZE / (1024 * 1024));
            m_isPlaying = false;
        }
        if (!m_isPlaying)
        {
#ifdef USE_WAV_HEADER
            UpdateSizes(ms_outputFile, ms_totalBytesRead); // update RIFF chunk size and data chunk size
#endif
            _stopCapture();
        }
    }
}

void AAudioRecorder::stopAAudioCapture()
{
    m_isPlaying = false;
}

void AAudioRecorder::_stopCapture()
{
    int32_t xRunCount = AAudioStream_getXRunCount(m_aaudioStream);
    ALOGI("AAudioStream_getXRunCount %d\n", xRunCount);
    aaudio_result_t result = AAudioStream_requestStop(m_aaudioStream);
    if (result == AAUDIO_OK)
    {
        aaudio_stream_state_t currentState = AAudioStream_getState(m_aaudioStream);
        aaudio_stream_state_t inputState = currentState;
        while (result == AAUDIO_OK && currentState != AAUDIO_STREAM_STATE_STOPPED)
        {
            result = AAudioStream_waitForStateChange(m_aaudioStream, inputState, &currentState, 60 * 1000 * 1000);
            inputState = currentState;
        }
    }
    else
    {
        ALOGE("aaudio request stop error, ret %d %s\n", result, AAudio_convertResultToText(result));
    }

    aaudio_stream_state_t currentState = AAudioStream_getState(m_aaudioStream);
    if (currentState != AAUDIO_STREAM_STATE_STOPPED)
    {
        ALOGW("AAudioStream_getState %s\n", AAudio_convertStreamStateToText(currentState));
    }
    if (m_aaudioStream != nullptr)
    {
        AAudioStream_close(m_aaudioStream);
        m_aaudioStream = nullptr;
    }
    if (ms_outputFile.is_open())
        ms_outputFile.close();
}

#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t
AAudioRecorder::dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
    // ALOGI("aaudio dataCallback, numFrames:%d, channelCount:%d, bytesPerChannel:%d\n", numFrames, m_channelCount,
    // bytesPerChannel);
    if (numFrames > 0)
    {
        if (ms_outputFile.is_open())
        {
            ms_outputFile.write(static_cast<const char *>(audioData), numFrames * ms_bytesPerFrame);
            // ALOGD("aaudio dataCallback, numFrames:%d\n", numFrames);
        }
        else
        {
            ALOGI("aaudio dataCallback end\n");
            return AAUDIO_CALLBACK_RESULT_STOP;
        }
        ms_totalBytesRead += numFrames * ms_bytesPerFrame;
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
