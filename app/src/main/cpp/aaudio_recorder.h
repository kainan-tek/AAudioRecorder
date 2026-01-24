// AAudio录音器头文件
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

// 日志标签
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
 * WAV文件写入类 (录音用)
 * 支持WAV文件的写入和音频数据保存
 */
class WavFileWriter {
public:
    WavFileWriter();
    ~WavFileWriter();

    // 以指定参数打开WAV文件进行写入
    bool open(const std::string& filePath, int32_t sampleRate, int32_t channelCount, aaudio_format_t format);

    // 关闭WAV文件
    void close();

    // 写入音频数据
    bool writeData(const void* data, size_t size);

    // 获取文件是否打开
    bool isOpen() const;

    // 获取每个采样的字节数
    static int32_t getBytesPerSample(aaudio_format_t format);

private:
    // WAV文件头定义
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

    std::string mFilePath;     // 文件路径
    std::ofstream mFileStream; // 文件流
    int32_t mSampleRate;       // 采样率
    int32_t mChannelCount;     // 声道数
    aaudio_format_t mFormat;   // 音频格式
    uint32_t mDataSize;        // 数据大小

    // 写入WAV文件头
    void writeHeader(uint32_t dataSize);
};

#endif

#endif // AAUDIO_RECORDER_H