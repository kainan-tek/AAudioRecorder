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
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// WAV文件读取类 (从AAudioPlayer移植)
class WaveFile {
public:
    // WAV文件头结构
    struct WaveHeader {
        // RIFF头
        char riffId[4];    // "RIFF"
        uint32_t riffSize; // 文件大小 - 8
        char waveId[4];    // "WAVE"

        // fmt子块
        char fmtId[4];          // "fmt "
        uint16_t audioFormat;   // 音频格式 (1 = PCM)
        uint16_t numChannels;   // 声道数
        uint32_t sampleRate;    // 采样率
        uint32_t byteRate;      // 字节率
        uint16_t blockAlign;    // 块对齐
        uint16_t bitsPerSample; // 每样本位数

        // data子块
        char dataId[4];    // "data"
        uint32_t dataSize; // 音频数据大小
    };

    WaveFile();
    ~WaveFile();

    // 打开WAV文件进行读取
    bool open(const std::string& filePath);
    
    // 关闭文件
    void close();
    
    // 读取音频数据
    size_t readAudioData(void* buffer, size_t bufferSize);
    
    // 检查文件是否打开
    bool isOpen() const;
    
    // 获取AAudio格式
    int32_t getAAudioFormat() const;
    
    // 获取格式信息字符串
    std::string getFormatInfo() const;
    
    // 获取采样率
    uint32_t getSampleRate() const { return header_.sampleRate; }
    
    // 获取声道数
    uint16_t getChannelCount() const { return header_.numChannels; }
    
    // 获取位深度
    uint16_t getBitsPerSample() const { return header_.bitsPerSample; }

private:
    std::ifstream file_;
    bool isOpen_;
    WaveHeader header_;

    bool readHeader();
    bool validateRiffHeader();
    bool readFmtChunk();
    bool findDataChunk();
    void skipChunk(uint32_t chunkSize);
    bool isValidFormat() const;
};

// WAV文件写入类 (录音用)
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

    // 获取文件路径
    const std::string& getFilePath() const;

    // 获取数据大小
    uint32_t getDataSize() const;

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

#endif // AAUDIO_RECORDER_H