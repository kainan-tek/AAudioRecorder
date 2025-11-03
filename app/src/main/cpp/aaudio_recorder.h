// 全新AAudio录音器头文件
#ifndef NEW_AAUDIO_RECORDER_H
#define NEW_AAUDIO_RECORDER_H

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
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

// WAV文件管理类
class WavFile {
public:
    WavFile();
    ~WavFile();

    // 以指定参数打开WAV文件
    bool open(const std::string& filePath, int32_t sampleRate, int32_t channelCount, aaudio_format_t format);

    // 关闭WAV文件
    void close();

    // 写入音频数据
    bool writeData(const void* data, size_t size);

    // 获取文件是否打开
    [[maybe_unused]] bool isOpen() const;

    // 获取文件路径
    [[maybe_unused]] const std::string& getFilePath() const;

    // 获取数据大小
    [[maybe_unused]] uint32_t getDataSize() const;

    // 获取每个采样的字节数
    static int32_t getBytesPerSample(aaudio_format_t format);

private:
    // WAV文件头定义
    struct WAVHeader {
        char chunkId[4];        // "RIFF"
        uint32_t chunkSize;     // 36 + subchunk2Size
        char format[4];         // "WAVE"
        char subchunk1Id[4];    // "fmt "
        uint32_t subchunk1Size; // 16 for PCM
        uint16_t audioFormat;   // 1 for PCM, 3 for IEEE float
        uint16_t numChannels;   // >0
        uint32_t sampleRate;    // 8000, 44100, etc.
        uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample / 8
        uint16_t blockAlign;    // numChannels * bitsPerSample / 8
        uint16_t bitsPerSample; // 8, 16, 24, 32
        char subchunk2Id[4];    // "data"
        uint32_t subchunk2Size; // numSamples * numChannels * bitsPerSample / 8
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

class AudioBuffer {
public:
    explicit AudioBuffer(size_t bufferSize);
    ~AudioBuffer() = default;

    bool writeToBuffer(const void* data, size_t size);
    // 读取数据 - size参数通过引用返回实际读取的字节数
    bool readFromBuffer(void* data, size_t& size);

    bool isEmpty() const;
    void notifyAll();

    // 设置所有者指针
    void setOwner(void* owner) { mOwner = owner; }

    // 调整缓冲区大小
    void reSize(size_t newSize);

private:
    std::vector<uint8_t> mBuffer;
    size_t mReadIndex = 0;  // 初始化为0
    size_t mWriteIndex = 0; // 初始化为0
    bool mIsFull = false;   // 初始化为false
    mutable std::mutex mMutex;
    std::condition_variable mNotEmpty;

    size_t getAvailableSpace() const;
    size_t getAvailableData() const;

    // 指向AAudioRecorder的指针，用于检查录音状态
    void* mOwner = nullptr;
};

// AAudio录音器主类
class AAudioRecorder {
public:
    AAudioRecorder();
    ~AAudioRecorder();

    bool initialize();
    bool startRecording();

    // 获取当前录音文件路径
    [[maybe_unused]] const std::string& getRecordedFilePath() const { return mRecordedFile; }

    // 设置录音文件路径
    [[maybe_unused]] void setRecordedFilePath(const std::string& filePath) { mRecordedFile = filePath; }
    bool stopRecording();
    void release();

    // 判断是否正在录音，供AudioBuffer使用
    bool isRecording() const {
        std::lock_guard<std::mutex> lock(mRecordingMutex);
        return mIsRecording;
    }

private:
    // AAudio相关
    bool mIsInitialized = false; // 初始化为false
    bool mIsRecording = false;   // 初始化为false

    aaudio_input_preset_t mInputPreset;
    int32_t mSampleRate;
    int32_t mChannelCount;
    aaudio_format_t mFormat;
    int32_t mBufferSizeInFrames;
    AAudioStream* mStream;
    AudioBuffer mAudioBuffer;
    std::string mRecordedFile; // 录音文件路径
    mutable std::mutex mRecordingMutex;
    WavFile mWavFile; // 录音文件管理对象

    // 线程相关
    std::thread mStreamReadThread; // 非回调模式：从音频流读取数据的线程
    std::thread mBufferReadThread; // 回调模式：从缓冲区读取数据并写入文件的线程

    // 回调模式的数据回调函数
    static aaudio_data_callback_result_t
    dataCallback([[maybe_unused]] AAudioStream* stream, void* userData, void* audioData, int32_t numFrames);

    // 错误回调函数
    static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error);

    // 非回调模式的音频读取线程
    void readFromStreamAndWriteToFile();

    // 回调模式的文件写入线程
    void readFromBufferAndWriteToFile();

    // 生成自动文件名
    std::string generateAutoFileName() const;
};

// JNI方法声明
extern "C" JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_AudioRecorderManager_nativeInit(JNIEnv* env,
                                                                                                      jobject thiz);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_AudioRecorderManager_nativeStartRecording(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_AudioRecorderManager_nativeStopRecording(JNIEnv* env, jobject thiz);

extern "C" JNIEXPORT void JNICALL Java_com_example_aaudiorecorder_AudioRecorderManager_nativeRelease(JNIEnv* env,
                                                                                                     jobject thiz);

#endif // NEW_AAUDIO_RECORDER_H