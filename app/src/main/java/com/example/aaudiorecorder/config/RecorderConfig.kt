package com.example.aaudiorecorder.config

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File

/**
 * AAudio录音配置数据类 - 参考AAudioPlayer的配置管理
 */
data class RecorderConfig(
    val inputPreset: String = "GENERIC",
    val sampleRate: Int = 48000,
    val channelCount: Int = 1,
    val format: Int = 16,
    val performanceMode: String = "LOW_LATENCY",
    val sharingMode: String = "SHARED",
    val outputPath: String = "",
    val description: String = "默认录音配置"
) {
    companion object {
        private const val TAG = "RecorderConfig"
        private const val CONFIG_FILE_PATH = "/data/recorder_configs.json"
        private const val ASSETS_CONFIG_FILE = "recorder_configs.json"
        
        fun loadConfigs(context: Context): List<RecorderConfig> {
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
        
        /**
         * 测试文件路径生成功能
         */
        fun testPathGeneration() {
            // 测试完整文件路径
            val config1 = RecorderConfig(outputPath = "/data/test_recording.wav")
            Log.d(TAG, "完整路径测试: ${config1.getActualOutputPath()}")
            
            // 测试目录路径
            val config2 = RecorderConfig(outputPath = "/data/", sampleRate = 44100, channelCount = 2, format = 16)
            Log.d(TAG, "目录路径测试: ${config2.getActualOutputPath()}")
            
            // 测试空路径
            val config3 = RecorderConfig(outputPath = "", sampleRate = 48000, channelCount = 1, format = 32)
            Log.d(TAG, "空路径测试: ${config3.getActualOutputPath()}")
        }

        private fun parseConfigs(jsonString: String): List<RecorderConfig> {
            val configsArray = JSONObject(jsonString).getJSONArray("configs")
            return (0 until configsArray.length()).map { i ->
                val config = configsArray.getJSONObject(i)
                RecorderConfig(
                    inputPreset = config.optString("inputPreset", "GENERIC"),
                    sampleRate = config.optInt("sampleRate", 48000),
                    channelCount = config.optInt("channelCount", 1),
                    format = config.optInt("format", 16),
                    performanceMode = config.optString("performanceMode", "LOW_LATENCY"),
                    sharingMode = config.optString("sharingMode", "SHARED"),
                    outputPath = config.optString("outputPath", ""),
                    description = config.optString("description", "录音配置")
                )
            }
        }
        
        private fun getDefaultConfigs(): List<RecorderConfig> {
            return listOf(
                RecorderConfig(
                    inputPreset = "GENERIC",
                    sampleRate = 48000,
                    channelCount = 1,
                    format = 16,
                    performanceMode = "LOW_LATENCY",
                    sharingMode = "SHARED",
                    outputPath = "/data/standard_recording.wav",
                    description = "标准录音 - 48kHz单声道"
                ),
                RecorderConfig(
                    inputPreset = "VOICE_COMMUNICATION",
                    sampleRate = 16000,
                    channelCount = 1,
                    format = 16,
                    performanceMode = "LOW_LATENCY",
                    sharingMode = "SHARED",
                    outputPath = "",
                    description = "语音通话 - 16kHz单声道"
                ),
                RecorderConfig(
                    inputPreset = "VOICE_RECOGNITION",
                    sampleRate = 16000,
                    channelCount = 1,
                    format = 16,
                    performanceMode = "LOW_LATENCY",
                    sharingMode = "SHARED",
                    outputPath = "/data/voice_recognition.wav",
                    description = "语音识别 - 16kHz单声道"
                ),
                RecorderConfig(
                    inputPreset = "CAMCORDER",
                    sampleRate = 44100,
                    channelCount = 2,
                    format = 16,
                    performanceMode = "POWER_SAVING",
                    sharingMode = "SHARED",
                    outputPath = "",
                    description = "摄像录音 - 44.1kHz立体声"
                ),
                RecorderConfig(
                    inputPreset = "VOICE_PERFORMANCE",
                    sampleRate = 48000,
                    channelCount = 1,
                    format = 16,
                    performanceMode = "LOW_LATENCY",
                    sharingMode = "EXCLUSIVE",
                    outputPath = "",
                    description = "高性能语音 - 48kHz独占模式"
                ),
                RecorderConfig(
                    inputPreset = "UNPROCESSED",
                    sampleRate = 48000,
                    channelCount = 2,
                    format = 16,
                    performanceMode = "LOW_LATENCY",
                    sharingMode = "EXCLUSIVE",
                    outputPath = "/data/raw_stereo_48k.wav",
                    description = "原始录音 - 48kHz立体声无处理"
                ),
                RecorderConfig(
                    inputPreset = "GENERIC",
                    sampleRate = 44100,
                    channelCount = 2,
                    format = 16,
                    performanceMode = "POWER_SAVING",
                    sharingMode = "SHARED",
                    outputPath = "",
                    description = "音乐录音 - 44.1kHz立体声"
                ),
                RecorderConfig(
                    inputPreset = "GENERIC",
                    sampleRate = 96000,
                    channelCount = 2,
                    format = 32,
                    performanceMode = "LOW_LATENCY",
                    sharingMode = "EXCLUSIVE",
                    outputPath = "",
                    description = "高保真录音 - 96kHz立体声32位"
                )
            )
        }
    }
    
    /**
     * 获取输入预设的整数值
     */
    fun getInputPresetValue(): Int {
        return when (inputPreset) {
            "GENERIC" -> 1
            "CAMCORDER" -> 5
            "VOICE_RECOGNITION" -> 6
            "VOICE_COMMUNICATION" -> 7
            "UNPROCESSED" -> 9
            "VOICE_PERFORMANCE" -> 10
            else -> 1 // 默认GENERIC
        }
    }
    
    /**
     * 获取格式的整数值
     */
    fun getFormatValue(): Int {
        return when (format) {
            16 -> 1  // AAUDIO_FORMAT_PCM_I16
            24 -> 2  // AAUDIO_FORMAT_PCM_I24_PACKED
            32 -> 3  // AAUDIO_FORMAT_PCM_I32
            else -> 1 // 默认PCM_I16
        }
    }
    
    /**
     * 获取性能模式的整数值
     */
    fun getPerformanceModeValue(): Int {
        return when (performanceMode) {
            "NONE" -> 10
            "POWER_SAVING" -> 11
            "LOW_LATENCY" -> 12
            else -> 12 // 默认LOW_LATENCY
        }
    }
    
    /**
     * 获取共享模式的整数值
     */
    fun getSharingModeValue(): Int {
        return when (sharingMode) {
            "EXCLUSIVE" -> 0
            "SHARED" -> 1
            else -> 1 // 默认SHARED
        }
    }
    
    /**
     * 获取实际的输出文件路径
     * 如果配置了完整路径则使用配置的路径，否则根据时间戳和音频参数自动生成
     */
    fun getActualOutputPath(): String {
        return if (outputPath.isNotEmpty() && outputPath.endsWith(".wav")) {
            // 配置了完整文件路径，直接使用
            outputPath
        } else {
            // 自动生成文件路径
            generateAutoFilePath()
        }
    }
    
    /**
     * 根据时间戳、采样率、声道数、格式自动生成文件路径
     */
    private fun generateAutoFilePath(): String {
        val timestamp = System.currentTimeMillis()
        val dateFormat = java.text.SimpleDateFormat("yyyyMMdd_HHmmss", java.util.Locale.getDefault())
        val timeString = dateFormat.format(java.util.Date(timestamp))
        
        val formatStr = when (format) {
            16 -> "16bit"
            24 -> "24bit" 
            32 -> "32bit"
            else -> "16bit"
        }
        
        val channelStr = if (channelCount == 1) "mono" else "${channelCount}ch"
        val sampleRateStr = "${sampleRate / 1000}k"
        
        val baseDir = if (outputPath.isNotEmpty() && !outputPath.endsWith(".wav")) {
            // 如果配置了目录路径，使用配置的目录
            outputPath.trimEnd('/')
        } else {
            // 默认使用 /data 目录
            "/data"
        }
        
        return "$baseDir/rec_${timeString}_${sampleRateStr}_${channelStr}_${formatStr}.wav"
    }
}