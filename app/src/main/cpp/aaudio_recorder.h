// AAudio recorder header file
#ifndef AAUDIO_RECORDER_H
#define AAUDIO_RECORDER_H

#include <condition_variable>
#include <fstream>
#include <jni.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <aaudio/AAudio.h>
#include <android/log.h>

// Log tags
#define LOG_TAG "AAudioRecorder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
// #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AAudio Recorder JNI Interface
 *
 * This header defines the JNI interface for the AAudio recorder functionality.
 * It provides functions to initialize, control, and manage audio recording
 * using Android's AAudio API with WAV file support.
 */

/**
 * Initialize the audio recorder
 * @param env JNI environment
 * @param thiz Java object instance
 * @return JNI_TRUE if initialization successful, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_initializeNative(JNIEnv* env,
                                                                                                    jobject thiz);

/**
 * Set native audio recording configuration
 * @param env JNI environment
 * @param thiz Java object instance
 * @param inputPreset Audio input preset
 * @param sampleRate Sample rate
 * @param channelCount Channel count
 * @param format Audio format
 * @param performanceMode Performance mode
 * @param sharingMode Sharing mode
 * @param outputPath Output file path
 * @return JNI_TRUE if configuration set successfully, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_setNativeConfig(JNIEnv* env,
                                                                                                   jobject thiz,
                                                                                                   jint inputPreset,
                                                                                                   jint sampleRate,
                                                                                                   jint channelCount,
                                                                                                   jint format,
                                                                                                   jint performanceMode,
                                                                                                   jint sharingMode,
                                                                                                   jstring outputPath);

/**
 * Start audio recording
 * @param env JNI environment
 * @param thiz Java object instance
 * @return JNI_TRUE if recording started successfully, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_startNativeRecording(JNIEnv* env,
                                                                                                        jobject thiz);

/**
 * Stop audio recording
 * @param env JNI environment
 * @param thiz Java object instance
 * @return JNI_TRUE if recording stopped successfully, JNI_FALSE otherwise
 */
JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(JNIEnv* env,
                                                                                                       jobject thiz);

/**
 * Release audio recorder resources
 * @param env JNI environment
 * @param thiz Java object instance
 */
JNIEXPORT void JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_releaseNative(JNIEnv* env, jobject thiz);

#ifdef __cplusplus
}

/**
 * WAV file writing class (for recording)
 * Supports WAV file writing and audio data saving
 */
class WavFileWriter {
public:
    WavFileWriter();
    ~WavFileWriter();

    // Open WAV file for writing with specified parameters
    bool open(const std::string& filePath, int32_t sampleRate, int32_t channelCount, aaudio_format_t format);

    // Close WAV file
    void close();

    // Write audio data
    bool writeData(const void* data, size_t size);

    // Get whether file is open
    bool isOpen() const;

    // Get bytes per sample
    static int32_t getBytesPerSample(aaudio_format_t format);

private:
    // WAV file header definition
    struct WAVHeader {
        char chunkId[4];                         // "RIFF"
        [[maybe_unused]] uint32_t chunkSize;     // 36 + subchunk2Size
        char format[4];                          // "WAVE"
        char subchunk1Id[4];                     // "fmt "
        [[maybe_unused]] uint32_t subchunk1Size; // 16 for PCM
        [[maybe_unused]] uint16_t audioFormat;   // 1 for PCM, 3 for IEEE float
        [[maybe_unused]] uint16_t numChannels;   // >0
        uint32_t sampleRate;                     // 8000, 44100, etc.
        [[maybe_unused]] uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample / 8
        [[maybe_unused]] uint16_t blockAlign;    // numChannels * bitsPerSample / 8
        [[maybe_unused]] uint16_t bitsPerSample; // 8, 16, 24, 32
        char subchunk2Id[4];                     // "data"
        [[maybe_unused]] uint32_t subchunk2Size; // numSamples * numChannels * bitsPerSample / 8
    };

    std::string mFilePath;     // File path
    std::ofstream mFileStream; // File stream
    int32_t mSampleRate;       // Sample rate
    int32_t mChannelCount;     // Channel count
    aaudio_format_t mFormat;   // Audio format
    uint32_t mDataSize;        // Data size

    // Write WAV file header
    void writeHeader(uint32_t dataSize);
};

#endif

#endif // AAUDIO_RECORDER_H