// 全新AAudio录音器实现
#include "aaudio_recorder.h"
#include <iomanip>
#include <sstream>

// 编译控制 - 启用回调模式
#define CALLBACK_MODE_ENABLE
// 默认缓冲区持续时间（毫秒）
#define DEFAULT_BUFFER_DURATION_MS 20

// WavFile类的实现
WavFile::WavFile() : mSampleRate(0), mChannelCount(0), mFormat(AAUDIO_FORMAT_INVALID), mDataSize(0) {}
WavFile::~WavFile() { close(); }

bool WavFile::open(const std::string& filePath, int32_t sampleRate, int32_t channelCount, aaudio_format_t format) {
    // 关闭已打开的文件
    close();

    // 以写模式打开新文件
    mFileStream.open(filePath, std::ios::binary | std::ios::out);
    if (!mFileStream.is_open()) {
        LOGE("Failed to open file for writing: %s", filePath.c_str());
        return false;
    }

    // 检查是否可写
    if (!mFileStream.good()) {
        LOGE("File stream not in good state after opening");
        close();
        return false;
    }

    // 保存参数
    mFilePath = filePath;
    mSampleRate = sampleRate;
    mChannelCount = channelCount;
    mFormat = format;
    mDataSize = 0;

    // 写入初始文件头（数据大小为0）
    writeHeader(0);

    return true;
}

void WavFile::close() {
    if (mFileStream.is_open()) {
        // 更新文件头，写入实际数据大小
        writeHeader(mDataSize);

        // 关闭文件
        mFileStream.close();

        // 重置状态
        mFilePath.clear();
        mSampleRate = 0;
        mChannelCount = 0;
        mFormat = AAUDIO_FORMAT_INVALID;
        mDataSize = 0;
        LOGI("Wav file closed: %s", mFilePath.c_str());
    }
}

bool WavFile::writeData(const void* data, size_t size) {
    if (!mFileStream.is_open() || data == nullptr || size == 0) {
        return false;
    }

    // 写入数据 - 显式转换size_t到std::streamsize并进行安全检查
    auto bytesToWrite = static_cast<std::streamsize>(size);
    if (bytesToWrite < 0 || static_cast<size_t>(bytesToWrite) != size) {
        LOGE("Data size too large for stream write operation");
        return false;
    }
    mFileStream.write(static_cast<const char*>(data), bytesToWrite);
    if (!mFileStream) {
        LOGE("Failed to write audio data to file");
        return false;
    }

    // 更新数据大小
    mDataSize += static_cast<uint32_t>(size);

    return true;
}

[[maybe_unused]] bool WavFile::isOpen() const { return mFileStream.is_open(); }

[[maybe_unused]] const std::string& WavFile::getFilePath() const { return mFilePath; }

[[maybe_unused]] uint32_t WavFile::getDataSize() const { return mDataSize; }

void WavFile::writeHeader(uint32_t dataSize) {
    if (!mFileStream.is_open()) {
        return;
    }

    WAVHeader header = {};
    std::memcpy(header.chunkId, "RIFF", 4);
    std::memcpy(header.format, "WAVE", 4);
    std::memcpy(header.subchunk1Id, "fmt ", 4);
    std::memcpy(header.subchunk2Id, "data", 4);

    header.subchunk1Size = 16; // PCM格式
    header.audioFormat = 1;    // PCM
    header.numChannels = static_cast<uint16_t>(mChannelCount);
    header.sampleRate = static_cast<uint32_t>(mSampleRate);
    header.bitsPerSample = static_cast<uint16_t>(getBytesPerSample(mFormat) * 8);
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.subchunk2Size = dataSize;
    header.chunkSize = 36 + dataSize;

    // 保存当前文件位置
    std::streampos currentPos = mFileStream.tellp();

    // 移动到文件开头并写入头信息
    mFileStream.seekp(0);
    mFileStream.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

    // 恢复到原来的位置
    if (currentPos > 0) {
        mFileStream.seekp(currentPos);
    }
}

int32_t WavFile::getBytesPerSample(aaudio_format_t format) {
    switch (format) {
    case AAUDIO_FORMAT_PCM_I16:
        return 2;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        return 3;
    case AAUDIO_FORMAT_PCM_I32:
    case AAUDIO_FORMAT_PCM_FLOAT:
        return 4;
    default:
        return 2; // 默认使用16位PCM
    }
}

// AudioBuffer类的实现
AudioBuffer::AudioBuffer(size_t bufferSize) : mBuffer(bufferSize), mReadIndex(0), mWriteIndex(0), mIsFull(false) {}

bool AudioBuffer::writeToBuffer(const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(mMutex);

    if (mIsFull) {
        LOGE("Buffer is full, cannot write data");
        return false;
    }

    const auto* src = static_cast<const uint8_t*>(data);
    size_t availableSpace = getAvailableSpace();
    size_t actualWriteSize = std::min(size, availableSpace);

    if (actualWriteSize == 0) {
        return false;
    }

    // 写入数据（考虑环形缓冲区的环绕情况）
    size_t firstPart = std::min(actualWriteSize, mBuffer.size() - mWriteIndex);
    std::memcpy(mBuffer.data() + mWriteIndex, src, firstPart);

    if (actualWriteSize > firstPart) {
        std::memcpy(mBuffer.data(), src + firstPart, actualWriteSize - firstPart);
        mWriteIndex = actualWriteSize - firstPart;
    } else {
        mWriteIndex += firstPart;
    }

    if (mWriteIndex == mBuffer.size()) {
        mWriteIndex = 0;
    }

    // 检查缓冲区是否已满
    if (getAvailableSpace() == 0) {
        mIsFull = true;
        LOGV("AudioBuffer is full");
    }

    // LOGV("AudioBuffer::write - Wrote %zu bytes, available space: %zu", actualWriteSize, getAvailableSpace());
    mNotEmpty.notify_one();
    return true;
}

bool AudioBuffer::readFromBuffer(void* data, size_t& size) {
    std::unique_lock<std::mutex> lock(mMutex);

    // 等待条件：缓冲区非空或录音已停止
    mNotEmpty.wait(lock, [this] {
        // 首先检查缓冲区是否有数据
        if (!isEmpty()) {
            return true;
        }

        // 然后检查录音是否已停止（如果有owner）
        auto* recorder = static_cast<AAudioRecorder*>(mOwner);
        return recorder && !recorder->isRecording();
    });

    auto* dest = static_cast<uint8_t*>(data);
    size_t availableData = getAvailableData();
    size_t actualReadSize = std::min(size, availableData);

    if (actualReadSize == 0) {
        LOGV("AudioBuffer::read - No data available, exiting read");
        size = 0;
        return false;
    }

    // LOGV("AudioBuffer::read - Reading %zu bytes, available data: %zu", actualReadSize, availableData);

    // 读取数据（考虑环形缓冲区的环绕情况）
    size_t firstPart = std::min(actualReadSize, mBuffer.size() - mReadIndex);
    std::memcpy(dest, mBuffer.data() + mReadIndex, firstPart);

    if (actualReadSize > firstPart) {
        std::memcpy(dest + firstPart, mBuffer.data(), actualReadSize - firstPart);
        mReadIndex = actualReadSize - firstPart;
    } else {
        mReadIndex += firstPart;
    }

    if (mReadIndex == mBuffer.size()) {
        mReadIndex = 0;
    }

    // 缓冲区不再是满的
    mIsFull = false;

    // 更新传入的size参数，使其反映实际读取的字节数
    size = actualReadSize;

    // LOGV("AudioBuffer::read - Completed, read %zu bytes, new available data: %zu", size, getAvailableData());
    return true;
}

bool AudioBuffer::isEmpty() const { return mReadIndex == mWriteIndex && !mIsFull; }

void AudioBuffer::notifyAll() { mNotEmpty.notify_all(); }

void AudioBuffer::reSize(size_t newSize) {
    std::lock_guard<std::mutex> lock(mMutex);

    // 调整缓冲区大小
    mBuffer.resize(newSize);

    // 重置缓冲区状态
    mReadIndex = 0;
    mWriteIndex = 0;
    mIsFull = false;

    LOGI("AudioBuffer size changed to %zu bytes", newSize);
}

size_t AudioBuffer::getAvailableSpace() const {
    if (mWriteIndex >= mReadIndex) {
        return mBuffer.size() - mWriteIndex + mReadIndex;
    } else {
        return mReadIndex - mWriteIndex;
    }
}

size_t AudioBuffer::getAvailableData() const {
    if (mIsFull) {
        return mBuffer.size();
    }
    if (mWriteIndex >= mReadIndex) {
        return mWriteIndex - mReadIndex;
    } else {
        return mBuffer.size() - mReadIndex + mWriteIndex;
    }
}

// AAudioRecorder类的实现
AAudioRecorder::AAudioRecorder()
    : mInputPreset(AAUDIO_INPUT_PRESET_GENERIC),
      mSampleRate(48000),
      mChannelCount(1),
      mFormat(AAUDIO_FORMAT_PCM_I16),
      mBufferSizeInFrames(0),
      mStream(nullptr),
      mAudioBuffer(1024), // will resize later
      mRecordedFile("/data/record_48k_1ch_16bit.wav"),
      mRecordingMutex() {
    // 设置AudioBuffer的所有者为当前AAudioRecorder对象
    mAudioBuffer.setOwner(this);
}

AAudioRecorder::~AAudioRecorder() { release(); }

bool AAudioRecorder::initialize() {
    LOGI("Initializing AAudioRecorder");

    if (mIsInitialized) {
        LOGI("AAudioRecorder already initialized");
        return true;
    }

    // 创建AAudio流构建器
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Failed to create AAudio stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    // 配置录音流参数
    AAudioStreamBuilder_setInputPreset(builder, mInputPreset);
    AAudioStreamBuilder_setSampleRate(builder, mSampleRate);
    AAudioStreamBuilder_setChannelCount(builder, mChannelCount);
    AAudioStreamBuilder_setFormat(builder, mFormat);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, DEFAULT_BUFFER_DURATION_MS * mSampleRate / 1000 * 3);

#ifdef CALLBACK_MODE_ENABLE
    // 设置回调函数
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);
#endif

    // 打开流
    result = AAudioStreamBuilder_openStream(builder, &mStream);
    if (result != AAUDIO_OK) {
        LOGE("Failed to open AAudio stream: %s", AAudio_convertResultToText(result));
        AAudioStreamBuilder_delete(builder);
        return false;
    }

    // 获取实际的流参数
    mSampleRate = AAudioStream_getSampleRate(mStream);
    mChannelCount = AAudioStream_getChannelCount(mStream);
    mFormat = AAudioStream_getFormat(mStream);
    mBufferSizeInFrames = AAudioStream_getBufferSizeInFrames(mStream);

    LOGI("AAudioRecorder initialized with sampleRate=%d, channelCount=%d, "
         "format=%d, bufferSizeInFrames=%d",
         mSampleRate, mChannelCount, mFormat, mBufferSizeInFrames);

    // 释放构建器
    AAudioStreamBuilder_delete(builder);
    mIsInitialized = true;
    LOGI("AAudioRecorder initialized successfully");
    return true;
}

bool AAudioRecorder::startRecording() {
    // 多次点击无效 - 如果正在录音，直接返回false
    if (mIsRecording) {
        LOGW("Already recording, cannot start again");
        return false;
    }
    LOGI("AAudioRecorder::startRecording");

    // 确保已初始化
    if (!mIsInitialized && !initialize()) {
        LOGE("Failed to initialize AAudioRecorder");
        return false;
    }

    // 如果mRecordedFile为空，则自动生成文件名，否则使用已设置的文件名
    if (mRecordedFile.empty()) {
        mRecordedFile = generateAutoFileName();
        LOGI("Auto-generated recording filename: %s", mRecordedFile.c_str());
    } else {
        LOGI("Using specified recording filename: %s", mRecordedFile.c_str());
    }

    // 使用WavFile打开文件
    if (!mWavFile.open(mRecordedFile, mSampleRate, mChannelCount, mFormat)) {
        LOGE("Failed to open file for recording: %s", mRecordedFile.c_str());
        return false;
    }

    // 重置缓冲区
    mAudioBuffer.reSize(mBufferSizeInFrames * mChannelCount * WavFile::getBytesPerSample(mFormat) * 4);

    // 启动AAudio流
    aaudio_result_t result = AAudioStream_requestStart(mStream);
    if (result != AAUDIO_OK) {
        LOGE("Failed to start AAudio stream: %s", AAudio_convertResultToText(result));
        mWavFile.close();
        return false;
    }

    { // 作用域限制，确保锁在创建线程前释放
        std::lock_guard<std::mutex> lock(mRecordingMutex);
        mIsRecording = true;
        LOGI("Recording state set to true");
    }
    LOGI("Started recording to file: %s", mRecordedFile.c_str());

#ifdef CALLBACK_MODE_ENABLE
    // 回调模式下启动文件写入线程
    mBufferReadThread = std::thread(&AAudioRecorder::readFromBufferAndWriteToFile, this);
#else
    // 非回调模式下启动读取线程
    mStreamReadThread = std::thread(&AAudioRecorder::readFromStreamAndWriteToFile, this);
#endif

    return true;
}

bool AAudioRecorder::stopRecording() {
    // 多次点击无效 - 如果不在录音，直接返回false
    {
        std::lock_guard<std::mutex> lock(mRecordingMutex);
        if (!mIsRecording) {
            LOGW("Not recording, cannot stop");
            return false;
        }

        mIsRecording = false;
        LOGI("Recording state set to false");
    }

    // 停止AAudio流
    aaudio_result_t result = AAudioStream_requestStop(mStream);
    if (result != AAUDIO_OK) {
        LOGE("Failed to stop AAudio stream: %s", AAudio_convertResultToText(result));
    }

    // 通知所有等待的线程
    LOGI("Notifying all waiting threads to wake up");
    mAudioBuffer.notifyAll();

    // 等待线程结束
#ifdef CALLBACK_MODE_ENABLE
    if (mBufferReadThread.joinable()) {
        mBufferReadThread.join();
        LOGI("Buffer read thread joined successfully");
    }
#else
    if (mStreamReadThread.joinable()) {
        mStreamReadThread.join();
        LOGI("Stream read thread joined successfully");
    }
#endif

    mWavFile.close(); // 关闭文件（会自动更新文件头）
    LOGI("Stopped recording, file saved: %s", mRecordedFile.c_str());

    return true;
}

void AAudioRecorder::release() {
    LOGI("Releasing AAudioRecorder resources");

    if (mIsRecording) {
        LOGI("Still recording, stopping recording first");
        stopRecording();
    }

    if (mStream != nullptr) {
        LOGI("Closing AAudio stream");
        AAudioStream_close(mStream);
        mStream = nullptr;
        mIsInitialized = false;
        LOGI("AAudio stream released");
    } else {
        LOGV("No active AAudio stream to release");
    }

    LOGI("AAudioRecorder resources released completely");
}

// AAudioRecorder类的私有方法实现
aaudio_data_callback_result_t
AAudioRecorder::dataCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    auto* recorder = static_cast<AAudioRecorder*>(userData);

    if (!recorder->mIsRecording) {
        LOGV("dataCallback: Recording stopped, returning STOP");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // 计算音频数据大小
    int32_t bytesPerSample = WavFile::getBytesPerSample(recorder->mFormat);
    size_t dataSize = static_cast<size_t>(numFrames) * recorder->mChannelCount * bytesPerSample;

    // 将数据写入缓冲区
    recorder->mAudioBuffer.writeToBuffer(audioData, dataSize);
    // LOGV("dataCallback: Processed %d frames (%zu bytes)", numFrames, dataSize);

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AAudioRecorder::errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio stream error: %s", AAudio_convertResultToText(error));
}

void AAudioRecorder::readFromStreamAndWriteToFile() {
    int32_t bytesPerSample = WavFile::getBytesPerSample(mFormat);
    std::vector<uint8_t> buffer(mBufferSizeInFrames * mChannelCount * bytesPerSample);

    LOGI("Audio read thread started (non-callback mode)");
    while (mIsRecording) {
        // 从流中读取数据
        int32_t framesRead = AAudioStream_read(mStream, buffer.data(), mBufferSizeInFrames,
                                               mBufferSizeInFrames / (mSampleRate / 1000) * 2);
        if (framesRead > 0) {
            // 写入文件
            size_t dataSize = static_cast<size_t>(framesRead) * mChannelCount * bytesPerSample;
            mWavFile.writeData(buffer.data(), dataSize);
            // LOGV("Audio read thread: Read and wrote %d frames (%zu bytes)", framesRead, dataSize);
        } else if (framesRead == AAUDIO_ERROR_DISCONNECTED) {
            LOGE("AAudio stream disconnected, exiting read thread");
            break;
        } else if (framesRead < 0) {
            LOGE("Error reading audio data: %s", AAudio_convertResultToText(framesRead));
            // 短暂休眠后继续
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            LOGV("Audio read thread: No frames read (timeout)");
        }
    }
}

void AAudioRecorder::readFromBufferAndWriteToFile() {
    int32_t bytesPerSample = WavFile::getBytesPerSample(mFormat);
    std::vector<uint8_t> buffer(mBufferSizeInFrames * mChannelCount * bytesPerSample);

    LOGI("readFromBufferAndWriteToFile thread started");
    // 缓冲区读取和写入的循环
    while (true) {
        // 从缓冲区读取数据并写入文件
        size_t bytesToRead = buffer.size();
        bool readResult = mAudioBuffer.readFromBuffer(buffer.data(), bytesToRead);
        if (readResult && bytesToRead > 0) {
            mWavFile.writeData(buffer.data(), bytesToRead);
            // LOGV("File write thread: Wrote %zu bytes to file", bytesToRead);
        } else if (bytesToRead == 0) {
            LOGV("File write thread: No more data to read, exiting loop");
            break;
        }
    }

    LOGV("File write thread: Processing remaining data in buffer");
    // 处理剩余数据
    while (!mAudioBuffer.isEmpty()) {
        size_t bytesToRead = buffer.size();
        if (mAudioBuffer.readFromBuffer(buffer.data(), bytesToRead) && bytesToRead > 0) {
            mWavFile.writeData(buffer.data(), bytesToRead);
        }
    }
}

std::string AAudioRecorder::generateAutoFileName() const {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};

    // Android平台使用localtime_r
    localtime_r(&now_c, &local_tm);

    // 根据音频格式确定位深
    int bitDepth = 16; // 默认16位
    if (mFormat == AAUDIO_FORMAT_PCM_I24_PACKED) {
        bitDepth = 24;
    } else if (mFormat == AAUDIO_FORMAT_PCM_I32 || mFormat == AAUDIO_FORMAT_PCM_FLOAT) {
        bitDepth = 32;
    }

    std::ostringstream oss;
    oss << "/data/recording_" << mSampleRate << "Hz_" << mChannelCount << "ch_" << bitDepth << "bit_"
        << std::put_time(&local_tm, "%Y%m%d_%H%M%S") << ".wav";

    return oss.str();
}

// 全局录音器实例
static AAudioRecorder* gAudioRecorder = nullptr;

// JNI方法实现
extern "C" JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_AudioRecorderManager_nativeInit(JNIEnv* env,
                                                                                                      jobject thiz) {
    if (gAudioRecorder == nullptr) {
        gAudioRecorder = new AAudioRecorder();
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_AudioRecorderManager_nativeStartRecording(JNIEnv* env, jobject thiz) {
    if (gAudioRecorder == nullptr) {
        LOGE("Audio recorder not initialized");
        return JNI_FALSE;
    }

    bool result = gAudioRecorder->startRecording();
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_AudioRecorderManager_nativeStopRecording(JNIEnv* env, jobject thiz) {
    if (gAudioRecorder == nullptr) {
        LOGE("Audio recorder not initialized");
        return JNI_FALSE;
    }

    return gAudioRecorder->stopRecording() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_aaudiorecorder_AudioRecorderManager_nativeRelease(JNIEnv* env,
                                                                                                     jobject thiz) {
    if (gAudioRecorder != nullptr) {
        gAudioRecorder->release();
        delete gAudioRecorder;
        gAudioRecorder = nullptr;
    }
}
