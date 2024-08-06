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
                                   m_isRecording(false),
                                   m_aaudioStream(nullptr),
                                   m_audioFile("/data/record_48k_1ch_16bit.wav")
{
#ifdef ENABLE_CALLBACK
    m_sharedBuf = new SharedBuffer(static_cast<size_t>(m_sampleRate / 1000 * 40 * m_channelCount * 2));
#endif
}

AAudioRecorder::~AAudioRecorder() = default;

void get_format_time(char *);
bool AAudioRecorder::startAAudioCapture()
{
    AAudioStreamBuilder *builder{nullptr};
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudio_createStreamBuilder() returned %d %s\n", result, AAudio_convertResultToText(result));
        return false;
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
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, (void *)m_sharedBuf);
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
        return false;
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

    int32_t bytesPerFrame = _getBytesPerSample(actualDataFormat) * actualChannelCount;
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
    m_audioFile = std::string(audioFileArr);
    ALOGI("Audio file path: %s\n", m_audioFile.c_str());

    /************** open output file **************/
    std::ofstream outputFile(m_audioFile, std::ios::binary | std::ios::out);
    if (!outputFile.is_open() || outputFile.fail())
    {
        ALOGE("AAudioRecorder error opening file\n");
        AAudioStream_close(m_aaudioStream);
        return false;
    }

#ifdef USE_WAV_HEADER
    /************** write audio file header **************/
    int32_t numSamples = 0;
    if (!writeWAVHeader(outputFile, numSamples, actualSampleRate, actualChannelCount,
                        bytesPerFrame / actualChannelCount * 8))
    {
        ALOGE("writeWAVHeader failed\n");
        AAudioStream_close(m_aaudioStream);
        outputFile.close();
        return false;
    }
#endif

    result = AAudioStream_requestStart(m_aaudioStream);
    if (result != AAUDIO_OK)
    {
        ALOGE("AAudioStream_requestStart returned %d %s\n", result, AAudio_convertResultToText(result));
        if (m_aaudioStream != nullptr)
        {
            AAudioStream_close(m_aaudioStream);
            m_aaudioStream = nullptr;
        }
        return false;
    }
    aaudio_stream_state_t state = AAudioStream_getState(m_aaudioStream);
    ALOGI("after request start, state = %s\n", AAudio_convertStreamStateToText(state));

#ifdef ENABLE_CALLBACK
    m_sharedBuf->setBufSize(m_framesPerBurst * bytesPerFrame * 8);
#endif
    m_isRecording = true;
    char *bufWrite2File = new char[m_framesPerBurst * bytesPerFrame * 2];
    while (m_aaudioStream)
    {
#ifdef ENABLE_CALLBACK
        usleep(8 * 1000);
        bool ret = m_sharedBuf->consume(bufWrite2File, m_framesPerBurst * bytesPerFrame * 2);
        if (ret)
        {
            outputFile.write(bufWrite2File, m_framesPerBurst * bytesPerFrame);
        }
#else
        int32_t framesRead = AAudioStream_read(m_aaudioStream, (void *)bufWrite2File, m_framesPerBurst, 60 * 1000 * 1000);
        if (framesRead)
        {
            // ALOGD("aaudio read, framesRead:%d, framesPerBurst:%d\n", framesRead, m_framesPerBurst);
            outputFile.write(bufWrite2File, framesRead * bytesPerFrame);
        }
#endif
        int64_t totalBytesRead = AAudioStream_getFramesRead(m_aaudioStream) * bytesPerFrame;
        if (totalBytesRead >= MAX_DATA_SIZE)
        {
            ALOGE("AudioRecord data size exceeds limit: %d MB, stop record\n", MAX_DATA_SIZE / (1024 * 1024));
            m_isRecording = false;
        }
        if (!m_isRecording)
        {
#ifdef USE_WAV_HEADER
            UpdateSizes(outputFile, totalBytesRead); // update RIFF chunk size and data chunk size
#endif
            _stopCapture();
            if (outputFile.is_open())
            {
                outputFile.close();
            }
            if (bufWrite2File)
            {
                delete[] bufWrite2File;
                bufWrite2File = nullptr;
            }
        }
    }
    return true;
}

bool AAudioRecorder::stopAAudioCapture()
{
    m_isRecording = false;
    return true;
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
}

#ifdef ENABLE_CALLBACK
aaudio_data_callback_result_t
AAudioRecorder::dataCallback(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
    if (numFrames > 0)
    {
        int32_t channels = AAudioStream_getChannelCount(stream);
        int32_t bytesPerFrame = _getBytesPerSample(AAudioStream_getFormat(stream)) * channels;
        bool ret = ((SharedBuffer *)userData)->produce((char *)audioData, numFrames * bytesPerFrame);
        if (!ret)
        {
            ALOGD("can't write to buffer, buffer is full\n");
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AAudioRecorder::errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error)
{
    ALOGE("errorCallback\n");
}
#endif

int32_t AAudioRecorder::_getBytesPerSample(aaudio_format_t format)
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

void get_format_time(char *format_time)
{
    time_t t = time(nullptr);
    struct tm *now = localtime(&t);
    strftime(format_time, 32, "%Y%m%d_%H.%M.%S", now);
}

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
