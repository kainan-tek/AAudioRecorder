package com.example.aaudiorecorder.config

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File

/**
 * AAudio recording configuration data class - based on AAudioPlayer configuration management
 */
data class AAudioConfig(
    val inputPreset: String = "AAUDIO_INPUT_PRESET_GENERIC",
    val sampleRate: Int = 48000,
    val channelCount: Int = 1,
    val format: String = "AAUDIO_FORMAT_PCM_I16",
    val performanceMode: String = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
    val sharingMode: String = "AAUDIO_SHARING_MODE_SHARED",
    val outputPath: String = "",
    val description: String = "Default Recording Configuration"
) {
    companion object {
        private const val TAG = "AAudioConfig"
        private const val CONFIG_FILE_PATH = "/data/aaudio_recorder_configs.json"
        private const val ASSETS_CONFIG_FILE = "aaudio_recorder_configs.json"
        
        fun loadConfigs(context: Context): List<AAudioConfig> {
            val externalFile = File(CONFIG_FILE_PATH)
            return try {
                val jsonString = if (externalFile.exists()) {
                    Log.i(TAG, "Loading recording configuration from external file")
                    externalFile.readText()
                } else {
                    Log.i(TAG, "Loading recording configuration from assets")
                    context.assets.open(ASSETS_CONFIG_FILE).bufferedReader().use { it.readText() }
                }
                parseConfigs(jsonString)
            } catch (e: Exception) {
                Log.e(TAG, "Failed to load recording configurations", e)
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
                    description = config.optString("description", "Recording Configuration")
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
                    description = "Standard Recording - 48kHz Mono"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION",
                    sampleRate = 16000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "",
                    description = "Voice Communication - 16kHz Mono"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_VOICE_RECOGNITION",
                    sampleRate = 16000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "/data/voice_recognition.wav",
                    description = "Voice Recognition - 16kHz Mono"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_CAMCORDER",
                    sampleRate = 48000,
                    channelCount = 2,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = "",
                    description = "Camcorder Recording - 48kHz Stereo"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE",
                    sampleRate = 48000,
                    channelCount = 1,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_EXCLUSIVE",
                    outputPath = "",
                    description = "High Performance Voice - 48kHz Exclusive Mode"
                ),
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_UNPROCESSED",
                    sampleRate = 48000,
                    channelCount = 2,
                    format = "AAUDIO_FORMAT_PCM_I16",
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_EXCLUSIVE",
                    outputPath = "/data/raw_stereo_48k.wav",
                    description = "Raw Recording - 48kHz Stereo Unprocessed"
                )
            )
        }
    }
    
    /**
     * Get input preset integer value
     */
    fun getInputPresetValue(): Int {
        return when (inputPreset) {
            "AAUDIO_INPUT_PRESET_GENERIC" -> 1
            "AAUDIO_INPUT_PRESET_CAMCORDER" -> 5
            "AAUDIO_INPUT_PRESET_VOICE_RECOGNITION" -> 6
            "AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION" -> 7
            "AAUDIO_INPUT_PRESET_UNPROCESSED" -> 9
            "AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE" -> 10
            else -> 1 // Default GENERIC
        }
    }
    
    /**
     * Get format integer value
     * Based on definitions in AAudio.h:
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
            else -> 1 // Default PCM_I16
        }
    }
    
    /**
     * Get performance mode integer value
     * Based on definitions in AAudio.h:
     * AAUDIO_PERFORMANCE_MODE_NONE = 10
     * AAUDIO_PERFORMANCE_MODE_POWER_SAVING = 11
     * AAUDIO_PERFORMANCE_MODE_LOW_LATENCY = 12
     */
    fun getPerformanceModeValue(): Int {
        return when (performanceMode) {
            "AAUDIO_PERFORMANCE_MODE_NONE" -> 10
            "AAUDIO_PERFORMANCE_MODE_POWER_SAVING" -> 11
            "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY" -> 12
            else -> 12 // Default LOW_LATENCY
        }
    }
    
    /**
     * Get sharing mode integer value
     * Based on definitions in AAudio.h:
     * AAUDIO_SHARING_MODE_EXCLUSIVE = 0
     * AAUDIO_SHARING_MODE_SHARED = 1
     */
    fun getSharingModeValue(): Int {
        return when (sharingMode) {
            "AAUDIO_SHARING_MODE_EXCLUSIVE" -> 0
            "AAUDIO_SHARING_MODE_SHARED" -> 1
            else -> 1 // Default SHARED
        }
    }
}