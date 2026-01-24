package com.example.aaudiorecorder.config

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File

/**
 * AAudio录音配置数据类 - 参考AAudioPlayer的配置管理
 */
data class AAudioConfig(
    val inputPreset: String = "AAUDIO_INPUT_PRESET_GENERIC",
    val sampleRate: Int = 48000,
    val channelCount: Int = 1,
    val format: String = "AAUDIO_FORMAT_PCM_I16",
    val performanceMode: String = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
    val sharingMode: String = "AAUDIO_SHARING_MODE_SHARED",
    val outputPath: String = "",
    val description: String = "默认录音配置"
) {
    companion object {
        private const val TAG = "AAudioConfig"
        private const val CONFIG_FILE_PATH = "/data/aaudio_recorder_configs.json"
        private const val ASSETS_CONFIG_FILE = "aaudio_recorder_configs.json"
        
        fun loadConfigs(context: Context): List<AAudioConfig> {
            val externalFile = File(CONFIG_FILE_PATH)
            return try {
                val jsonString = if (externalFile.exists()) {
                    Log.i(TAG, "从外部文件加载录音配置")
                    externalFile.readText()
                } else {
                    Log.i(TAG, "从assets加载录音配置")
                    context.assets.open(ASSETS_CONFIG_FILE).bufferedReader().use { it.readText() }
                }
                parseConfigs(jsonString)
            } catch (e: Exception) {
                Log.e(TAG, "加载录音配置失败", e)
                getDefaultConfigs()
            }
        }
        
        private fun parseConfigs(jsonString: String): List<AAudioConfig> {
            val configsArray = JSONObject(jsonString).getJSONArray("configs")
            return (0 until configsArray.length()).map { i ->
                val config = configsArray.getJSONObject(i)
                AAudioConfig(
                    inputPreset = config.optString("inputPreset", "AAUDIO_INPUT_PRESET_GENERIC"),
                    sampleRate = config.optInt("sampleRate", 48000),
                    channelCount = config.optInt("channelCount", 1),
                    format = config.optString("format", "AAUDIO_FORMAT_PCM_I16"),
                    performanceMode = config.optString("performanceMode", "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY"),
                    sharingMode = config.optString("sharingMode", "AAUDIO_SHARING_MODE_SHARED"),
                    outputPath = config.optString("outputPath", ""),
                    description = config.optString("description", "录音配置")
                )
            }
        }
        
        private fun getDefaultConfigs(): List<AAudioConfig> {
            return listOf(
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_GENERIC",
                    sampleRate = 48000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "/data/standard_recording.wav",
                    description = "标准录音 - 48kHz单声道"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION",
                    sampleRate = 16000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "",
                    description = "语音通话 - 16kHz单声道"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_VOICE_RECOGNITION",
                    sampleRate = 16000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "/data/voice_recognition.wav",
                    description = "语音识别 - 16kHz单声道"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_CAMCORDER",
                    sampleRate = 44100,
                    channelCount = 2,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "",
                    description = "摄像录音 - 44.1kHz立体声"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE",
                    sampleRate = 48000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_EXCLUSIVE",
                    outputPath = "",
                    description = "高性能语音 - 48kHz独占模式"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_UNPROCESSED",
                    sampleRate = 48000,
                    channelCount = 2,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_EXCLUSIVE",
                    outputPath = "/data/raw_stereo_48k.wav",
                    description = "原始录音 - 48kHz立体声无处理"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_GENERIC",
                    sampleRate = 44100,
                    channelCount = 2,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "",
                    description = "音乐录音 - 44.1kHz立体声"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_GENERIC",
                    sampleRate = 96000,
                    channelCount = 2,
                    format = "AAUDIO_FORMAT_PCM_FLOAT",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_EXCLUSIVE",
                    outputPath = "",
                    description = "高保真录音 - 96kHz立体声32位浮点"
                )
            )
        }
    }
    
    /**
     * 获取输入预设的整数值
     */
    fun getInputPresetValue(): Int {
        return when (inputPreset) {
            "AAUDIO_INPUT_PRESET_GENERIC" -> 1
            "AAUDIO_INPUT_PRESET_CAMCORDER" -> 5
            "AAUDIO_INPUT_PRESET_VOICE_RECOGNITION" -> 6
            "AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION" -> 7
            "AAUDIO_INPUT_PRESET_UNPROCESSED" -> 9
            "AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE" -> 10
            else -> 1 // 默认GENERIC
        }
    }
    
    /**
     * 获取格式的整数值
     * 基于 AAudio.h 中的定义：
     * AAUDIO_FORMAT_PCM_I16 = 1
     * AAUDIO_FORMAT_PCM_FLOAT = 2  
     * AAUDIO_FORMAT_PCM_I24_PACKED = 3
     * AAUDIO_FORMAT_PCM_I32 = 4
     */
    fun getFormatValue(): Int {
        return when (format) {
            "AAUDIO_FORMAT_PCM_I16" -> 1
            "AAUDIO_FORMAT_PCM_FLOAT" -> 2
            "AAUDIO_FORMAT_PCM_I24_PACKED" -> 3
            "AAUDIO_FORMAT_PCM_I32" -> 4
            else -> 1 // 默认PCM_I16
        }
    }
    
    /**
     * 获取性能模式的整数值
     * 基于 AAudio.h 中的定义：
     * AAUDIO_PERFORMANCE_MODE_NONE = 10
     * AAUDIO_PERFORMANCE_MODE_POWER_SAVING = 11
     * AAUDIO_PERFORMANCE_MODE_LOW_LATENCY = 12
     */
    fun getPerformanceModeValue(): Int {
        return when (performanceMode) {
            "AAUDIO_PERFORMANCE_MODE_NONE" -> 10
            "AAUDIO_PERFORMANCE_MODE_POWER_SAVING" -> 11
            "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY" -> 12
            else -> 12 // 默认LOW_LATENCY
        }
    }
    
    /**
     * 获取共享模式的整数值
     * 基于 AAudio.h 中的定义：
     * AAUDIO_SHARING_MODE_EXCLUSIVE = 0
     * AAUDIO_SHARING_MODE_SHARED = 1
     */
    fun getSharingModeValue(): Int {
        return when (sharingMode) {
            "AAUDIO_SHARING_MODE_EXCLUSIVE" -> 0
            "AAUDIO_SHARING_MODE_SHARED" -> 1
            else -> 1 // 默认SHARED
        }
    }
}