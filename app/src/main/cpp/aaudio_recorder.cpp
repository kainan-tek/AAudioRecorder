#include "aaudio_recorder.h"
#include <jni.h>
#include <atomic>
#include <memory>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

// 日志宏
#define LOG_TAG "AAudioRecorder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 简化的录音器状态结构
struct AudioRecorderState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WavFileWriter> wavWriter;
    std::atomic<bool> isRecording{false};

    // Java回调相关
    JavaVM* jvm = nullptr;
    jobject recorderInstance = nullptr;
    jmethodID onRecordingStartedMethod = nullptr;
    jmethodID onRecordingStoppedMethod = nullptr;
    jmethodID onRecordingErrorMethod = nullptr;

    // 配置参数
    int32_t inputPreset = 1; // AAUDIO_INPUT_PRESET_GENERIC
    int32_t sampleRate = 48000;
    int32_t channelCount = 1;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;
    aaudio_performance_mode_t performanceMode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    std::string outputPath = "/data/";
    
    // 最后录音文件信息
    std::string lastRecordingPath;
    int64_t lastRecordingSize = 0;
};

static AudioRecorderState g_recorder;

// 简化的Java回调
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

// 生成录音文件名或使用配置的完整路径
static std::string getRecordingFilePath() {
    // 如果outputPath已经是完整的文件路径（以.wav结尾），直接使用
    if (!g_recorder.outputPath.empty() && 
        (g_recorder.outputPath.length() > 4) &&
        (g_recorder.outputPath.substr(g_recorder.outputPath.length() - 4) == ".wav")) {
        return g_recorder.outputPath;
    }
    
    // 否则生成自动文件名
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    
    // 使用配置的目录路径，如果为空则使用默认路径
    std::string baseDir = g_recorder.outputPath.empty() ? "/data" : g_recorder.outputPath;
    
    // 确保目录路径以/结尾
    if (!baseDir.empty() && baseDir.back() != '/') {
        baseDir += "/";
    }
    
    oss << baseDir;
    oss << "rec_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    oss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    oss << "_" << (g_recorder.sampleRate / 1000) << "k";
    oss << "_" << (g_recorder.channelCount == 1 ? "mono" : std::to_string(g_recorder.channelCount) + "ch");
    
    // 格式标识
    switch (g_recorder.format) {
        case AAUDIO_FORMAT_PCM_I16:
            oss << "_16bit";
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

// 音频回调函数
static aaudio_data_callback_result_t
audioCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    if (!g_recorder.isRecording.load(std::memory_order_acquire)) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_recorder.wavWriter || !g_recorder.wavWriter->isOpen()) {
        LOGE("WAV writer not available");
        g_recorder.isRecording.store(false, std::memory_order_release);
        notifyRecordingError("WAV文件写入器未打开");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    // 计算需要写入的字节数
    int32_t channelCount = AAudioStream_getChannelCount(stream);
    int32_t bytesPerSample = (AAudioStream_getFormat(stream) == AAUDIO_FORMAT_PCM_I16) ? 2 : 4;
    int32_t bytesToWrite = numFrames * channelCount * bytesPerSample;

    // 写入音频数据到WAV文件
    if (!g_recorder.wavWriter->writeData(audioData, static_cast<size_t>(bytesToWrite))) {
        LOGE("Failed to write audio data to WAV file");
        g_recorder.isRecording.store(false, std::memory_order_release);
        notifyRecordingError("写入音频数据失败");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// 错误回调函数
static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error callback: %s", AAudio_convertResultToText(error));
    g_recorder.isRecording.store(false, std::memory_order_release);
    
    // 构建错误消息
    std::string errorMsg = "录音流错误: ";
    errorMsg += AAudio_convertResultToText(error);
    notifyRecordingError(errorMsg);
}

// 创建录音流
static bool createRecordingStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    
    if (result != AAUDIO_OK) {
        LOGE("Failed to create stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    // 配置录音流
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSampleRate(builder, g_recorder.sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, g_recorder.channelCount);
    AAudioStreamBuilder_setFormat(builder, g_recorder.format);
    AAudioStreamBuilder_setPerformanceMode(builder, g_recorder.performanceMode);
    AAudioStreamBuilder_setSharingMode(builder, g_recorder.sharingMode);
    AAudioStreamBuilder_setInputPreset(builder, static_cast<aaudio_input_preset_t>(g_recorder.inputPreset));
    
    // 设置回调
    AAudioStreamBuilder_setDataCallback(builder, audioCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    // 创建流
    result = AAudioStreamBuilder_openStream(builder, &g_recorder.stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        LOGE("Failed to open recording stream: %s", AAudio_convertResultToText(result));
        return false;
    }

    // 获取实际的流参数
    int32_t actualSampleRate = AAudioStream_getSampleRate(g_recorder.stream);
    int32_t actualChannelCount = AAudioStream_getChannelCount(g_recorder.stream);
    aaudio_format_t actualFormat = AAudioStream_getFormat(g_recorder.stream);

    LOGI("Recording stream created - Sample Rate: %d, Channels: %d, Format: %d", 
         actualSampleRate, actualChannelCount, actualFormat);

    // 更新实际参数
    g_recorder.sampleRate = actualSampleRate;
    g_recorder.channelCount = actualChannelCount;
    g_recorder.format = actualFormat;

    return true;
}

// JNI方法实现
extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_initializeNative(JNIEnv *env, jobject thiz) {
    LOGI("Initializing AAudio recorder");
    
    // 保存Java对象引用
    if (g_recorder.jvm == nullptr) {
        env->GetJavaVM(&g_recorder.jvm);
    }
    
    if (g_recorder.recorderInstance != nullptr) {
        env->DeleteGlobalRef(g_recorder.recorderInstance);
    }
    g_recorder.recorderInstance = env->NewGlobalRef(thiz);
    
    // 获取回调方法ID
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == nullptr) {
        LOGE("Failed to get object class");
        return JNI_FALSE;
    }
    
    g_recorder.onRecordingStartedMethod = env->GetMethodID(clazz, "onNativeRecordingStarted", "()V");
    g_recorder.onRecordingStoppedMethod = env->GetMethodID(clazz, "onNativeRecordingStopped", "()V");
    g_recorder.onRecordingErrorMethod = env->GetMethodID(clazz, "onNativeRecordingError", "(Ljava/lang/String;)V");
    
    if (!g_recorder.onRecordingStartedMethod || !g_recorder.onRecordingStoppedMethod || !g_recorder.onRecordingErrorMethod) {
        LOGE("Failed to get callback method IDs");
        return JNI_FALSE;
    }
    
    env->DeleteLocalRef(clazz);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_setNativeConfig(JNIEnv *env, jobject thiz,
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
    
    g_recorder.inputPreset = inputPreset;
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
    
    LOGI("Config updated - SR: %d, CH: %d, Format: %d, Path: %s", 
         sampleRate, channelCount, format, g_recorder.outputPath.c_str());
    
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_startNativeRecording(JNIEnv *env, jobject thiz) {
    if (g_recorder.isRecording.load()) {
        LOGW("Already recording");
        return JNI_FALSE;
    }
    
    LOGI("Starting recording");
    
    // 创建录音流
    if (!createRecordingStream()) {
        notifyRecordingError("创建录音流失败");
        return JNI_FALSE;
    }
    
    // 获取文件路径并创建WAV写入器
    std::string filePath = getRecordingFilePath();
    g_recorder.wavWriter = std::make_unique<WavFileWriter>();
    
    if (!g_recorder.wavWriter->open(filePath, g_recorder.sampleRate, g_recorder.channelCount, g_recorder.format)) {
        LOGE("Failed to open WAV file: %s", filePath.c_str());
        notifyRecordingError("创建录音文件失败");
        return JNI_FALSE;
    }
    
    // 启动录音流
    aaudio_result_t result = AAudioStream_requestStart(g_recorder.stream);
    if (result != AAUDIO_OK) {
        LOGE("Failed to start recording stream: %s", AAudio_convertResultToText(result));
        g_recorder.wavWriter->close();
        g_recorder.wavWriter.reset();
        notifyRecordingError("启动录音流失败");
        return JNI_FALSE;
    }
    
    g_recorder.isRecording.store(true);
    g_recorder.lastRecordingPath = filePath;
    
    LOGI("Recording started successfully: %s", filePath.c_str());
    notifyRecordingStarted();
    
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(JNIEnv *env, jobject thiz) {
    if (!g_recorder.isRecording.load(std::memory_order_acquire)) {
        LOGW("Not recording");
        return JNI_FALSE;
    }
    
    LOGI("Stopping recording");
    
    // 首先设置停止标志
    g_recorder.isRecording.store(false, std::memory_order_release);
    
    // 等待一小段时间让回调函数完成
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 停止录音流
    if (g_recorder.stream) {
        aaudio_result_t result = AAudioStream_requestStop(g_recorder.stream);
        if (result != AAUDIO_OK) {
            LOGW("Failed to stop stream: %s", AAudio_convertResultToText(result));
        }
        
        result = AAudioStream_close(g_recorder.stream);
        if (result != AAUDIO_OK) {
            LOGW("Failed to close stream: %s", AAudio_convertResultToText(result));
        }
        g_recorder.stream = nullptr;
    }
    
    // 关闭WAV文件
    if (g_recorder.wavWriter) {
        g_recorder.lastRecordingSize = static_cast<int64_t>(g_recorder.wavWriter->getDataSize());
        g_recorder.wavWriter->close();
        g_recorder.wavWriter.reset();
    }
    
    LOGI("Recording stopped successfully. File: %s, Size: %lld bytes", 
         g_recorder.lastRecordingPath.c_str(), static_cast<long long>(g_recorder.lastRecordingSize));
    
    notifyRecordingStopped();
    
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_getLastRecordingPath(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(g_recorder.lastRecordingPath.c_str());
}

JNIEXPORT jlong JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_getLastRecordingSize(JNIEnv *env, jobject thiz) {
    return g_recorder.lastRecordingSize;
}

JNIEXPORT void JNICALL
Java_com_example_aaudiorecorder_recorder_AAudioRecorder_releaseNative(JNIEnv *env, jobject thiz) {
    LOGI("Releasing AAudio recorder");
    
    // 停止录音
    if (g_recorder.isRecording.load()) {
        Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(env, thiz);
    }
    
    // 清理Java引用
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