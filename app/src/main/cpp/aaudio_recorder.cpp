#include "aaudio_recorder.h"

#include <sys/stat.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>

struct AudioRecorderState {
    AAudioStream* stream = nullptr;
    std::unique_ptr<WavFile> wav_file;
    std::atomic<bool> is_recording{false};

    JavaVM* jvm = nullptr;
    jobject recorder_instance = nullptr;
    jmethodID on_recording_started_method = nullptr;
    jmethodID on_recording_stopped_method = nullptr;
    jmethodID on_recording_error_method = nullptr;

    aaudio_input_preset_t input_preset = AAUDIO_INPUT_PRESET_GENERIC;
    int32_t sample_rate = 48000;
    int32_t channel_count = 1;
    aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;
    aaudio_performance_mode_t performance_mode = AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
    aaudio_sharing_mode_t sharing_mode = AAUDIO_SHARING_MODE_SHARED;
    std::string output_path = "/data/recorded_48k_1ch_16bit.wav";
    bool auto_generate_filename = false;
};

namespace {

AudioRecorderState g_recorder;

} // namespace

static void notifyRecordingStarted() {
    if (g_recorder.jvm && g_recorder.recorder_instance && g_recorder.on_recording_started_method) {
        JNIEnv* env;
        if (g_recorder.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_recorder.recorder_instance, g_recorder.on_recording_started_method);
        }
    }
}

static void notifyRecordingStopped() {
    if (g_recorder.jvm && g_recorder.recorder_instance && g_recorder.on_recording_stopped_method) {
        JNIEnv* env;
        if (g_recorder.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            env->CallVoidMethod(g_recorder.recorder_instance, g_recorder.on_recording_stopped_method);
        }
    }
}

static void notifyRecordingError(const std::string& error) {
    if (g_recorder.jvm && g_recorder.recorder_instance && g_recorder.on_recording_error_method) {
        JNIEnv* env;
        if (g_recorder.jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            jstring error_str = env->NewStringUTF(error.c_str());
            env->CallVoidMethod(g_recorder.recorder_instance, g_recorder.on_recording_error_method, error_str);
            env->DeleteLocalRef(error_str);
        }
    }
}

static std::string generateTimestampedFilePath() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf;
    localtime_r(&now_time_t, &tm_buf);

    std::ostringstream oss;
    oss << g_recorder.output_path << "/";
    oss << "rec_";
    oss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S");
    oss << "_" << std::setfill('0') << std::setw(3) << now_ms.count();
    oss << "_" << (g_recorder.sample_rate / 1000) << "k";
    oss << "_" << g_recorder.channel_count << "ch";

    int bits_per_sample = 16;
    switch (g_recorder.format) {
    case AAUDIO_FORMAT_PCM_I16:
        bits_per_sample = 16;
        break;
    case AAUDIO_FORMAT_PCM_FLOAT:
        bits_per_sample = 32;
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        bits_per_sample = 24;
        break;
    case AAUDIO_FORMAT_PCM_I32:
        bits_per_sample = 32;
        break;
    default:
        bits_per_sample = 16;
        break;
    }
    oss << "_" << bits_per_sample << "bit";
    oss << ".wav";

    return oss.str();
}

static std::string getFinalRecordingFilePath() {
    if (g_recorder.auto_generate_filename) {
        return generateTimestampedFilePath();
    }
    return g_recorder.output_path;
}

static aaudio_data_callback_result_t
audioCallback(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    if (!g_recorder.is_recording.load(std::memory_order_acquire)) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    if (!g_recorder.wav_file || !g_recorder.wav_file->isOpen()) {
        LOGE("WAV file not available");
        g_recorder.is_recording.store(false, std::memory_order_release);
        notifyRecordingError("[FILE] WAV file not opened");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    int32_t channel_count = AAudioStream_getChannelCount(stream);
    int32_t bytes_per_sample;

    switch (AAudioStream_getFormat(stream)) {
    case AAUDIO_FORMAT_PCM_I16:
        bytes_per_sample = 2;
        break;
    case AAUDIO_FORMAT_PCM_FLOAT:
        bytes_per_sample = 4;
        break;
    case AAUDIO_FORMAT_PCM_I24_PACKED:
        bytes_per_sample = 3;
        break;
    case AAUDIO_FORMAT_PCM_I32:
        bytes_per_sample = 4;
        break;
    default:
        bytes_per_sample = 2;
        break;
    }

    int32_t bytes_to_write = numFrames * channel_count * bytes_per_sample;

    if (!g_recorder.wav_file->writeData(audioData, static_cast<size_t>(bytes_to_write))) {
        LOGE("Failed to write audio data to WAV file");
        g_recorder.is_recording.store(false, std::memory_order_release);
        notifyRecordingError("[FILE] Failed to write audio data");
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static void errorCallback(AAudioStream* stream, void* userData, aaudio_result_t error) {
    LOGE("AAudio error callback: %s", AAudio_convertResultToText(error));
    g_recorder.is_recording.store(false, std::memory_order_release);

    std::string error_msg = "[STREAM] Recording stream error: ";
    error_msg += AAudio_convertResultToText(error);
    notifyRecordingError(error_msg);
}

static bool createAAudioStream() {
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);

    if (result != AAUDIO_OK) {
        LOGE("Failed to create stream builder: %s", AAudio_convertResultToText(result));
        return false;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSampleRate(builder, g_recorder.sample_rate);
    AAudioStreamBuilder_setChannelCount(builder, g_recorder.channel_count);
    AAudioStreamBuilder_setFormat(builder, g_recorder.format);
    AAudioStreamBuilder_setPerformanceMode(builder, g_recorder.performance_mode);
    AAudioStreamBuilder_setSharingMode(builder, g_recorder.sharing_mode);
    AAudioStreamBuilder_setInputPreset(builder, g_recorder.input_preset);

    AAudioStreamBuilder_setDataCallback(builder, audioCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    result = AAudioStreamBuilder_openStream(builder, &g_recorder.stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        LOGE("Failed to open recording stream: %s", AAudio_convertResultToText(result));
        return false;
    }

    int32_t actual_sample_rate = AAudioStream_getSampleRate(g_recorder.stream);
    int32_t actual_channel_count = AAudioStream_getChannelCount(g_recorder.stream);
    aaudio_format_t actual_format = AAudioStream_getFormat(g_recorder.stream);

    LOGI("Recording stream created - Sample Rate: %d, Channels: %d, Format: %d", actual_sample_rate,
         actual_channel_count, actual_format);

    g_recorder.sample_rate = actual_sample_rate;
    g_recorder.channel_count = actual_channel_count;
    g_recorder.format = actual_format;

    return true;
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_initializeNative(JNIEnv* env,
                                                                                                    jobject thiz) {
    LOGI("Initializing AAudio recorder");

    if (g_recorder.jvm == nullptr) {
        env->GetJavaVM(&g_recorder.jvm);
    }

    if (g_recorder.recorder_instance != nullptr) {
        env->DeleteGlobalRef(g_recorder.recorder_instance);
    }
    g_recorder.recorder_instance = env->NewGlobalRef(thiz);

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == nullptr) {
        LOGE("Failed to get object class");
        return JNI_FALSE;
    }

    g_recorder.on_recording_started_method = env->GetMethodID(clazz, "onNativeRecordingStarted", "()V");
    g_recorder.on_recording_stopped_method = env->GetMethodID(clazz, "onNativeRecordingStopped", "()V");
    g_recorder.on_recording_error_method = env->GetMethodID(clazz, "onNativeRecordingError", "(Ljava/lang/String;)V");

    if (!g_recorder.on_recording_started_method || !g_recorder.on_recording_stopped_method ||
        !g_recorder.on_recording_error_method) {
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

    g_recorder.input_preset = static_cast<aaudio_input_preset_t>(inputPreset);
    g_recorder.sample_rate = sampleRate;
    g_recorder.channel_count = channelCount;
    g_recorder.format = static_cast<aaudio_format_t>(format);
    g_recorder.performance_mode = static_cast<aaudio_performance_mode_t>(performanceMode);
    g_recorder.sharing_mode = static_cast<aaudio_sharing_mode_t>(sharingMode);

    const char* path_str = env->GetStringUTFChars(outputPath, nullptr);
    if (path_str != nullptr) {
        g_recorder.output_path = path_str;
        // Check if path ends with .wav - if not, it's a directory path and we need to generate filename
        g_recorder.auto_generate_filename =
            (g_recorder.output_path.length() < 4 ||
             g_recorder.output_path.substr(g_recorder.output_path.length() - 4) != ".wav");
        env->ReleaseStringUTFChars(outputPath, path_str);
    } else {
        LOGE("Failed to get output path string");
        return JNI_FALSE;
    }

    LOGI("Config updated - SR: %d, CH: %d, Format: %d, Path: %s, AutoGenerate: %s", sampleRate, channelCount, format,
         g_recorder.output_path.c_str(), g_recorder.auto_generate_filename ? "true" : "false");

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_startNativeRecording(JNIEnv* env,
                                                                                                        jobject thiz) {
    if (g_recorder.is_recording.load()) {
        LOGW("Already recording");
        return JNI_FALSE;
    }

    LOGI("Starting recording");

    if (!createAAudioStream()) {
        notifyRecordingError("[STREAM] Failed to create recording stream");
        return JNI_FALSE;
    }

    std::string file_path = getFinalRecordingFilePath();
    g_recorder.wav_file = std::make_unique<WavFile>();

    if (!g_recorder.wav_file->open(file_path, g_recorder.sample_rate, g_recorder.channel_count, g_recorder.format)) {
        LOGE("Failed to open WAV file: %s", file_path.c_str());
        if (g_recorder.stream) {
            AAudioStream_close(g_recorder.stream);
            g_recorder.stream = nullptr;
        }
        g_recorder.wav_file.reset();
        notifyRecordingError("[FILE] Failed to create recording file");
        return JNI_FALSE;
    }

    aaudio_result_t result = AAudioStream_requestStart(g_recorder.stream);
    if (result != AAUDIO_OK) {
        LOGE("Failed to start recording stream: %s", AAudio_convertResultToText(result));
        g_recorder.wav_file->close();
        g_recorder.wav_file.reset();
        notifyRecordingError("[STREAM] Failed to start recording stream");
        return JNI_FALSE;
    }

    g_recorder.is_recording.store(true);

    LOGI("Recording started successfully: %s", file_path.c_str());
    notifyRecordingStarted();

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(JNIEnv* env,
                                                                                                       jobject thiz) {
    LOGI("Stopping recording");

    g_recorder.is_recording.store(false, std::memory_order_release);

    if (g_recorder.stream) {
        aaudio_result_t result = AAudioStream_requestStop(g_recorder.stream);
        if (result != AAUDIO_OK) {
            LOGW("Failed to request stop: %s", AAudio_convertResultToText(result));
        } else {
            aaudio_stream_state_t state = AAUDIO_STREAM_STATE_STOPPING;
            result =
                AAudioStream_waitForStateChange(g_recorder.stream, AAUDIO_STREAM_STATE_STOPPING, &state, 100000000);
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

    if (g_recorder.wav_file) {
        g_recorder.wav_file->close();
        g_recorder.wav_file.reset();
    }

    LOGI("Recording stopped successfully");

    notifyRecordingStopped();

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_aaudiorecorder_recorder_AAudioRecorder_releaseNative(JNIEnv* env,
                                                                                             jobject thiz) {
    LOGI("Releasing AAudio recorder");

    if (g_recorder.is_recording.load()) {
        Java_com_example_aaudiorecorder_recorder_AAudioRecorder_stopNativeRecording(env, thiz);
    }

    if (g_recorder.recorder_instance) {
        env->DeleteGlobalRef(g_recorder.recorder_instance);
        g_recorder.recorder_instance = nullptr;
    }

    g_recorder.jvm = nullptr;
    g_recorder.on_recording_started_method = nullptr;
    g_recorder.on_recording_stopped_method = nullptr;
    g_recorder.on_recording_error_method = nullptr;

    LOGI("AAudio recorder released");
}

} // extern "C"

WavFile::WavFile() : sample_rate_(0), channel_count_(0), format_(AAUDIO_FORMAT_PCM_I16), data_size_(0) {}

WavFile::~WavFile() noexcept { close(); }

bool WavFile::open(const std::string& file_path, int32_t sample_rate, int32_t channel_count, aaudio_format_t format) {
    close();

    file_path_ = file_path;
    sample_rate_ = sample_rate;
    channel_count_ = channel_count;
    format_ = format;
    data_size_ = 0;

    file_stream_.open(file_path, std::ios::binary | std::ios::out);
    if (!file_stream_.is_open()) {
        LOGE("Failed to open WAV file for writing: %s", file_path.c_str());
        return false;
    }

    writeHeader(0);

    LOGI("WAV file opened for writing: %s", file_path.c_str());
    return true;
}

void WavFile::close() {
    if (file_stream_.is_open()) {
        writeHeader(data_size_);
        file_stream_.close();
        LOGI("WAV file closed: %s, final size: %u bytes", file_path_.c_str(), data_size_);
    }
}

bool WavFile::writeData(const void* data, size_t size) {
    if (!file_stream_.is_open() || !data || size == 0) {
        return false;
    }

    file_stream_.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (file_stream_.fail()) {
        LOGE("Failed to write data to WAV file");
        return false;
    }

    data_size_ += static_cast<uint32_t>(size);
    return true;
}

bool WavFile::isOpen() const { return file_stream_.is_open(); }

int32_t WavFile::getBytesPerSample(aaudio_format_t format) {
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

void WavFile::writeHeader(uint32_t data_size) {
    if (!file_stream_.is_open()) {
        return;
    }

    WavHeader header = {};

    memcpy(header.chunk_id, "RIFF", 4);
    header.chunk_size = 36 + data_size;
    memcpy(header.format, "WAVE", 4);

    memcpy(header.subchunk1_id, "fmt ", 4);
    header.subchunk1_size = 16;

    if (format_ == AAUDIO_FORMAT_PCM_FLOAT) {
        header.audio_format = 3;
    } else {
        header.audio_format = 1;
    }

    header.num_channels = static_cast<uint16_t>(channel_count_);
    header.sample_rate = static_cast<uint32_t>(sample_rate_);
    header.bits_per_sample = static_cast<uint16_t>(getBytesPerSample(format_) * 8);
    header.block_align = static_cast<uint16_t>(channel_count_ * getBytesPerSample(format_));
    header.byte_rate = header.sample_rate * header.block_align;

    memcpy(header.subchunk2_id, "data", 4);
    header.subchunk2_size = data_size;

    file_stream_.seekp(0, std::ios::beg);
    file_stream_.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file_stream_.flush();
}
