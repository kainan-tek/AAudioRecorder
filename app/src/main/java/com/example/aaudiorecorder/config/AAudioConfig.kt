package com.example.aaudiorecorder.config

import android.content.Context
import android.util.Log
import com.example.aaudiorecorder.common.AAudioConstants
import org.json.JSONObject
import java.io.File

/**
 * AAudio recording configuration data class - optimized with validation
 */
data class AAudioConfig(
    val inputPreset: String = "AAUDIO_INPUT_PRESET_GENERIC",
    val sampleRate: Int = 48000,
    val channelCount: Int = 1,
    val format: Int = 16, // Use a bit of depth directly (16, 24, 32)
    val performanceMode: String = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
    val sharingMode: String = "AAUDIO_SHARING_MODE_SHARED",
    val outputPath: String = AAudioConstants.DEFAULT_RECORD_FILE,
    val description: String = "Default Recording Configuration"
) {
    
    init {
        // Validate parameters
        require(AAudioConstants.isValidSampleRate(sampleRate)) { 
            "Invalid sample rate: $sampleRate" 
        }
        require(AAudioConstants.isValidChannelCount(channelCount)) { 
            "Invalid channel count: $channelCount" 
        }
        require(AAudioConstants.isValidFormat(format)) { 
            "Invalid format bit depth: $format (must be 16, 24, or 32)" 
        }
    }
    
    companion object {
        private const val TAG = "AAudioConfig"
        
        fun loadConfigs(context: Context): List<AAudioConfig> {
            return try {
                val externalFile = File(AAudioConstants.CONFIG_FILE_PATH)
                val jsonString = if (externalFile.exists()) {
                    Log.i(TAG, "Loading recording configuration from external file")
                    externalFile.readText()
                } else {
                    Log.i(TAG, "Loading recording configuration from assets")
                    context.assets.open(AAudioConstants.ASSETS_CONFIG_FILE).bufferedReader().use { it.readText() }
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
                    format = config.optInt("format", 16),
                    performanceMode = config.optString("performanceMode", "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY"),
                    sharingMode = config.optString("sharingMode", "AAUDIO_SHARING_MODE_SHARED"),
                    outputPath = config.optString("outputPath", AAudioConstants.DEFAULT_RECORD_FILE),
                    description = config.optString("description", "Recording Configuration")
                )
            }
        }
        
        private fun getDefaultConfigs(): List<AAudioConfig> {
            Log.w(TAG, "Using hardcoded emergency configuration")
            return listOf(
                AAudioConfig(
                    inputPreset = "AAUDIO_INPUT_PRESET_GENERIC",
                    sampleRate = 48000,
                    channelCount = 1,
                    format = 16,
                    performanceMode = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
                    sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                    outputPath = AAudioConstants.DEFAULT_RECORD_FILE,
                    description = "Emergency Fallback - Generic Recording"
                )
            )
        }
    }
}