#include "aaudio_recorder.h"
#include <atomic>
#include <chrono>
#include <cstring> // for memcpy
#include <iomanip>
#include <jni.h>
#include <memory>
#include <sstream>
#include <string>

// Simplified recorder state structure
struct AudioRecorderState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WavFileWriter> wavWriter;
    std::atomic<bool> isRecording{false};

    // Java callback related
    JavaVM* jvm = nullptr;
    jobject recorderInstance = nullptr;
    jmethodID onRecordingStartedMethod = nullptr;
    jmethodID onRecordingStoppedMethod = nullptr;
    jmethodID onRecordingErrorMethod = nullptr;

    // Configuration parameters
    aaudio_input_preset_t inputPreset = AAUDIO_INPUT_PRESET_GENERIC;
    int32_t sampleRate = 48000;
    int32_t channelCount = 1;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;
    aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    std::string outputPath = "/data/";
};

static AudioRecorderState g_recorder;

// Simplified Java callbacks
static void notifyRecordingStarted() {
    if (g_recorder.jvm && g_recorder.recorderInstance && g_recorder.onRecordingStartedMethod) {
        JNIEnv* env;
        if (g_recorder.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_recorder.recorderInstance, g_recorder.onRecordingStartedMethod);
        }
    }
}

static void notifyRecordingStopped() {
    if (g_recorder.jvm && g_recorder.recorderInstance && g_recorder.onRecordingStoppedMethod) {
        JNIEnv* env;
        if (g_recorder.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_recorder.recorderInstance, g_recorder.onRecordingStoppedMethod);
        }
    }
}

static void notifyRecordingError(const std::string& error) {
    if (g_recorder.jvm && g_recorder.recorderInstance && g_recorder.onRecordingErrorMethod) {
        JNIEnv* env;
        if (g_recorder.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            jstring errorStr = env->NewStringUTF(error.c_str());
            env->CallVoidMethod(g_recorder.recorderInstance, g_recorder.onRecordingErrorMethod, errorStr);
            env->DeleteLocalRef(errorStr);
        }
    }
}

// Generate recording filename or use configured full path
static std::string getRecordingFilePath() {
    // If outputPath is already a complete file path (ending with .wav), use it directly
    if (!g_recorder.outputPath.empty() && (g_recorder.outputPath.length() > 4) &&
        (g_recorder.outputPath.substr(g_recorder.outputPath.length() - 4) == ".wav")) {
        return g_recorder.outputPath;
    }

    // Otherwise generate automatic filename (when outputPath is empty)
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;

    // Use default /data directory
    oss << "/data/";
    oss << "rec_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    oss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    oss << "_" << (g_recorder.sampleRate / 1000) << "k";
    oss << "_" << (g_recorder.channelCount == 1 ? "mono" : std::to_string(g_recorder.channelCount) + "ch");

    // Format identifier
    switch (g_recorder.format) {
    case AAUDIO_FORMAT_PCM_I16:
        oss << "_16bit";
        break;
    case AAUDIO_FORMAT_PCM_FLOAT:
        oss << "_float";
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        oss << "_24bit";
        break;
    case AAUDIO_FORMAT_PCM_I32:
        oss << "_32bit";
        break;
    default:
        oss << "_16bit";
        break;
    }

    oss << ".wav";

    return oss.str();
}

// Audio callback function
static aaudio_data_callback_result_t
audioCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    if (!g_recorder.isRecording.load(std::memory_order_acquire)) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_recorder.wavWriter || !g_recorder.wavWriter->isOpen()) {
        LOGE("WAV writer not available");
        g_recorder.isRecording.store(false, std::memory_order_release);
        notifyRecordingError("[FILE] WAV file writer not opened");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // Calculate bytes to write
    int32_t channelCount = AAudioStream_getChannelCount(stream);
    int32_t bytesPerSample;

    switch (AAudioStream_getFormat(stream)) {
    case AAUDIO_FORMAT_PCM_I16:
        bytesPerSample = 2;
        break;
    case AAUDIO_FORMAT_PCM_FLOAT:
        bytesPerSample = 4;
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        bytesPerSample = 3;
        break;
    case AAUDIO_FORMAT_PCM_I32:
        bytesPerSample = 4;
        break;
    default:
        bytesPerSample = 2; // Default to 16-bit
        break;
    }

    int32_t bytesToWrite = numFrames * channelCount * bytesPerSample;

    // Write audio data to WAV file
    if (!g_recorder.wavWriter->writeData(audioData, static_cast<size_t>(bytesToWrite))) {
        LOGE("Failed to write audio data to WAV file");
        g_recorder.isRecording.store(false, std::memory_order_release);
        notifyRecordingError("[FILE] Failed to write audio data");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// Error callback function
static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error callback: %s", AAudio_convertResultToText(error));
    g_recorder.isRecording.store(false, std::memory_order_release);

    // Build error message
    std::string errorMsg = "[STREAM] Recording stream error: ";
    errorMsg += AAudio_convertResultToText(error);
    notifyRecordingError(errorMsg);
}

// Create AAudio stream
static bool createAAudioStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);

    if (result != AAUDIO_OK) {
        LOGE("Failed to create stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    // Configure recording stream
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSampleRate(builder, g_recorder.sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, g_recorder.channelCount);
    AAudioStreamBuilder_setFormat(builder, g_recorder.format);
    AAudioStreamBuilder_setPerformanceMode(builder, g_recorder.performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, g_recorder.sharingMode);
    AAudioStreamBuilder_setInputPreset(builder, g_recorder.inputPreset);

    // Set callbacks
    AAudioStreamBuilder_setDataCallback(builder, audioCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    // Create stream
    result = AAudioStreamBuilder_openStream(builder, &g_recorder.stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        LOGE("Failed to open recording stream: %s", AAudio_convertResultToText(result));
        return false;
    }

    // Get actual stream parameters
    int32_t actualSampleRate = AAudioStream_getSampleRate(g_recorder.stream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(g_recorder.stream);
    aaudio_format_t actualFormat = AAudioStream_getFormat(g_recorder.stream);

    LOGI("Recording stream created - Sample Rate: %d, Channels: %d, Format: %d", actualSampleRate, actualChannelCount,
         actualFormat);

    // Update actual parameters
    g_recorder.sampleRate = actualSampleRate;
    g_recorder.channelCount = actualChannelCount;
    g_recorder.format = actualFormat;

    return true;
}

// JNI method implementations
extern "C" {

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_initializeNative(JNIEnv* env,
                                                                                                    jobject thiz) {
    LOGI("Initializing AAudio recorder");

    // Save Java object reference
    if (g_recorder.jvm == nullptr) {
        env->GetJavaVM(&g_recorder.jvm);
    }

    if (g_recorder.recorderInstance != nullptr) {
        env->DeleteGlobalRef(g_recorder.recorderInstance);
    }
    g_recorder.recorderInstance = env->NewGlobalRef(thiz);

    // Get callback method IDs
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == nullptr) {
        LOGE("Failed to get object class");
        return JNI_FALSE;
    }

    g_recorder.onRecordingStartedMethod = env->GetMethodID(clazz, "onNativeRecordingStarted", "()V");
    g_recorder.onRecordingStoppedMethod = env->GetMethodID(clazz, "onNativeRecordingStopped", "()V");
    g_recorder.onRecordingErrorMethod = env->GetMethodID(clazz, "onNativeRecordingError", "(Ljava/lang/String;)V");

    if (!g_recorder.onRecordingStartedMethod || !g_recorder.onRecordingStoppedMethod ||
        !g_recorder.onRecordingErrorMethod) {
        LOGE("Failed to get callback method IDs");
        return JNI_FALSE;
    }

    env->DeleteLocalRef(clazz);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_setNativeConfig(JNIEnv* env,
                                                                                                   jobject thiz,
                                                                                                   jint inputPreset,
                                                                                                   jint sampleRate,
                                                                                                   jint channelCount,
                                                                                                   jint format,
                                                                                                   jint performanceMode,
                                                                                                   jint sharingMode,
                                                                                                   jstring outputPath) {
    if (outputPath == nullptr) {
        LOGE("Output path is null");
        return JNI_FALSE;
    }

    g_recorder.inputPreset = static_cast<aaudio_input_preset_t>(inputPreset);
    g_recorder.sampleRate = sampleRate;
    g_recorder.channelCount = channelCount;
    g_recorder.format = static_cast<aaudio_format_t>(format);
    g_recorder.performanceMode = static_cast<aaudio_performance_mode_t>(performanceMode);
    g_recorder.sharingMode = static_cast<aaudio_sharing_mode_t>(sharingMode);

    const char* pathStr = env->GetStringUTFChars(outputPath, nullptr);
    if (pathStr != nullptr) {
        g_recorder.outputPath = pathStr;
        env->ReleaseStringUTFChars(outputPath, pathStr);
    } else {
        LOGE("Failed to get output path string");
        return JNI_FALSE;
    }

    LOGI("Config updated - SR: %d, CH: %d, Format: %d, Path: %s", sampleRate, channelCount, format,
         g_recorder.outputPath.c_str());

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_startNativeRecording(JNIEnv* env,
                                                                                                        jobject thiz) {
    if (g_recorder.isRecording.load()) {
        LOGW("Already recording");
        return JNI_FALSE;
    }

    LOGI("Starting recording");

    // Create AAudio stream
    if (!createAAudioStream()) {
        notifyRecordingError("[STREAM] Failed to create recording stream");
        return JNI_FALSE;
    }

    // Get file path and create WAV writer
    std::string filePath = getRecordingFilePath();
    g_recorder.wavWriter = std::make_unique<WavFileWriter>();

    if (!g_recorder.wavWriter->open(filePath, g_recorder.sampleRate, g_recorder.channelCount, g_recorder.format)) {
        LOGE("Failed to open WAV file: %s", filePath.c_str());
        // Close and cleanup AAudio stream since WAV file creation failed
        if (g_recorder.stream) {
            AAudioStream_close(g_recorder.stream);
            g_recorder.stream = nullptr;
        }
        g_recorder.wavWriter.reset();
        notifyRecordingError("[FILE] Failed to create recording file");
        return JNI_FALSE;
    }

    // Start recording stream
    aaudio_result_t result = AAudioStream_requestStart(g_recorder.stream);
    if (result != AAUDIO_OK) {
        LOGE("Failed to start recording stream: %s", AAudio_convertResultToText(result));
        g_recorder.wavWriter->close();
        g_recorder.wavWriter.reset();
        notifyRecordingError("[STREAM] Failed to start recording stream");
        return JNI_FALSE;
    }

    g_recorder.isRecording.store(true);

    LOGI("Recording started successfully: %s", filePath.c_str());
    notifyRecordingStarted();

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(JNIEnv* env,
                                                                                                       jobject thiz) {
    LOGI("Stopping recording");

    // First set stop flag to signal callback to stop
    g_recorder.isRecording.store(false, std::memory_order_release);

    // Stop recording stream
    if (g_recorder.stream) {
        aaudio_result_t result = AAudioStream_requestStop(g_recorder.stream);
        if (result != AAUDIO_OK) {
            LOGW("Failed to request stop: %s", AAudio_convertResultToText(result));
        } else {
            aaudio_stream_state_t state = AAUDIO_STREAM_STATE_STOPPING;
            result = AAudioStream_waitForStateChange(g_recorder.stream, AAUDIO_STREAM_STATE_STOPPING, &state,
                                                     100000000 // 100ms timeout in nanoseconds
            );
            if (result != AAUDIO_OK) {
                LOGW("Failed to wait for stop: %s", AAudio_convertResultToText(result));
            }
        }

        result = AAudioStream_close(g_recorder.stream);
        if (result != AAUDIO_OK) {
            LOGW("Failed to close stream: %s", AAudio_convertResultToText(result));
        }
        g_recorder.stream = nullptr;
    }

    // Close WAV file
    if (g_recorder.wavWriter) {
        g_recorder.wavWriter->close();
        g_recorder.wavWriter.reset();
    }

    LOGI("Recording stopped successfully");

    notifyRecordingStopped();

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_releaseNative(JNIEnv* env,
                                                                                             jobject thiz) {
    LOGI("Releasing AAudio recorder");

    // Stop recording
    if (g_recorder.isRecording.load()) {
        Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(env, thiz);
    }

    // Clean up Java references
    if (g_recorder.recorderInstance) {
        env->DeleteGlobalRef(g_recorder.recorderInstance);
        g_recorder.recorderInstance = nullptr;
    }

    g_recorder.jvm = nullptr;
    g_recorder.onRecordingStartedMethod = nullptr;
    g_recorder.onRecordingStoppedMethod = nullptr;
    g_recorder.onRecordingErrorMethod = nullptr;

    LOGI("AAudio recorder released");
}

} // extern "C"

// WavFileWriter class implementation
WavFileWriter::WavFileWriter() : sampleRate_(0), channelCount_(0), format_(AAUDIO_FORMAT_PCM_I16), dataSize_(0) {}

WavFileWriter::~WavFileWriter() noexcept { close(); }

bool WavFileWriter::open(const std::string& filePath,
                         int32_t sampleRate,
                         int32_t channelCount,
                         aaudio_format_t format) {
    close();

    filePath_ = filePath;
    sampleRate_ = sampleRate;
    channelCount_ = channelCount;
    format_ = format;
    dataSize_ = 0;

    fileStream_.open(filePath, std::ios::binary | std::ios::out);
    if (!fileStream_.is_open()) {
        LOGE("Failed to open WAV file for writing: %s", filePath.c_str());
        return false;
    }

    writeHeader(0);

    LOGI("WAV file opened for writing: %s", filePath.c_str());
    return true;
}

void WavFileWriter::close() {
    if (fileStream_.is_open()) {
        writeHeader(dataSize_);
        fileStream_.close();
        LOGI("WAV file closed: %s, final size: %u bytes", filePath_.c_str(), dataSize_);
    }
}

bool WavFileWriter::writeData(const void* data, size_t size) {
    if (!fileStream_.is_open() || !data || size == 0) {
        return false;
    }

    fileStream_.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (fileStream_.fail()) {
        LOGE("Failed to write data to WAV file");
        return false;
    }

    dataSize_ += static_cast<uint32_t>(size);
    return true;
}

bool WavFileWriter::isOpen() const { return fileStream_.is_open(); }

int32_t WavFileWriter::getBytesPerSample(aaudio_format_t format) {
    switch (format) {
    case AAUDIO_FORMAT_PCM_I16:
        return 2;
    case AAUDIO_FORMAT_PCM_FLOAT:
        return 4;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        return 3;
    case AAUDIO_FORMAT_PCM_I32:
        return 4;
    default:
        return 2;
    }
}

void WavFileWriter::writeHeader(uint32_t dataSize) {
    if (!fileStream_.is_open()) {
        return;
    }

    WAVHeader header = {};

    memcpy(header.chunkId, "RIFF", 4);
    header.chunkSize = 36 + dataSize;
    memcpy(header.format, "WAVE", 4);

    memcpy(header.subchunk1Id, "fmt ", 4);
    header.subchunk1Size = 16;

    if (format_ == AAUDIO_FORMAT_PCM_FLOAT) {
        header.audioFormat = 3;
    } else {
        header.audioFormat = 1;
    }

    header.numChannels = static_cast<uint16_t>(channelCount_);
    header.sampleRate = static_cast<uint32_t>(sampleRate_);
    header.bitsPerSample = static_cast<uint16_t>(getBytesPerSample(format_) * 8);
    header.blockAlign = static_cast<uint16_t>(channelCount_ * getBytesPerSample(format_));
    header.byteRate = header.sampleRate * header.blockAlign;

    memcpy(header.subchunk2Id, "data", 4);
    header.subchunk2Size = dataSize;

    fileStream_.seekp(0, std::ios::beg);
    fileStream_.write(reinterpret_cast<const char*>(&header), sizeof(header));
    fileStream_.flush();
}