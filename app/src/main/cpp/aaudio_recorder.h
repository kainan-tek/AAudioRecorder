// AAudio recorder header file
#ifndef AAUDIO_RECORDER_AAUDIO_RECORDER_H_
#define AAUDIO_RECORDER_AAUDIO_RECORDER_H_

#include <fstream>
#include <string>

#include <aaudio/AAudio.h>
#include <android/log.h>
#include <jni.h>

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
class WavFile {
public:
    WavFile();
    ~WavFile() noexcept;

    // Disable copy and assignment
    WavFile(const WavFile&) = delete;
    WavFile& operator=(const WavFile&) = delete;

    // Allow move
    WavFile(WavFile&&) noexcept = default;
    WavFile& operator=(WavFile&&) noexcept = default;

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
    struct WavHeader {
        char chunk_id[4];                          // "RIFF"
        [[maybe_unused]] uint32_t chunk_size;      // 36 + subchunk2_size
        char format[4];                            // "WAVE"
        char subchunk1_id[4];                      // "fmt "
        [[maybe_unused]] uint32_t subchunk1_size;  // 16 for PCM
        [[maybe_unused]] uint16_t audio_format;    // 1 for PCM, 3 for IEEE float
        [[maybe_unused]] uint16_t num_channels;    // >0
        uint32_t sample_rate;                      // 8000, 44100, etc.
        [[maybe_unused]] uint32_t byte_rate;       // sample_rate * num_channels * bits_per_sample / 8
        [[maybe_unused]] uint16_t block_align;     // num_channels * bits_per_sample / 8
        [[maybe_unused]] uint16_t bits_per_sample; // 8, 16, 24, 32
        char subchunk2_id[4];                      // "data"
        [[maybe_unused]] uint32_t subchunk2_size;  // num_samples * num_channels * bits_per_sample / 8
    };

    std::string file_path_;     // File path
    std::ofstream file_stream_; // File stream
    int32_t sample_rate_;       // Sample rate
    int32_t channel_count_;     // Channel count
    aaudio_format_t format_;    // Audio format
    uint32_t data_size_;        // Data size

    void writeHeader(uint32_t data_size);
};

#endif

#endif // AAUDIO_RECORDER_AAUDIO_RECORDER_H_