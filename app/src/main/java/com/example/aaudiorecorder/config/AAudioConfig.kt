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
    val inputPreset: String = AAudioConstants.INPUT_PRESET_GENERIC,
    val sampleRate: Int = 48000,
    val channelCount: Int = 1,
    val format: Int = 16, // Use bit depth directly (16, 24, 32)
    val performanceMode: String = AAudioConstants.PERFORMANCE_MODE_LOW_LATENCY,
    val sharingMode: String = AAudioConstants.SHARING_MODE_SHARED,
    val outputPath: String = "",
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
    
    /**
     * Get input preset integer value
     */
    fun getInputPresetValue(): Int = AAudioConstants.getInputPresetValue(inputPreset)
    
    /**
     * Get format integer value
     */
    fun getFormatValue(): Int = AAudioConstants.getFormatValueFromBitDepth(format)
    
    /**
     * Get performance mode integer value
     */
    fun getPerformanceModeValue(): Int = AAudioConstants.getPerformanceModeValue(performanceMode)
    
    /**
     * Get sharing mode integer value
     */
    fun getSharingModeValue(): Int = AAudioConstants.getSharingModeValue(sharingMode)
    
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
                    inputPreset = config.optString("inputPreset", AAudioConstants.INPUT_PRESET_GENERIC),
                    sampleRate = config.optInt("sampleRate", 48000),
                    channelCount = config.optInt("channelCount", 1),
                    format = config.optInt("format", 16),
                    performanceMode = config.optString("performanceMode", AAudioConstants.PERFORMANCE_MODE_LOW_LATENCY),
                    sharingMode = config.optString("sharingMode", AAudioConstants.SHARING_MODE_SHARED),
                    outputPath = config.optString("outputPath", ""),
                    description = config.optString("description", "Recording Configuration")
                )
            }
        }
        
        private fun getDefaultConfigs(): List<AAudioConfig> {
            // 直接返回硬编码的emergency配置，不再重复尝试读取assets
            Log.w(TAG, "Using hardcoded emergency configuration")
            return listOf(
                AAudioConfig(
                    inputPreset = AAudioConstants.INPUT_PRESET_GENERIC,
                    sampleRate = 48000,
                    channelCount = 1,
                    format = 16,
                    performanceMode = AAudioConstants.PERFORMANCE_MODE_LOW_LATENCY,
                    sharingMode = AAudioConstants.SHARING_MODE_SHARED,
                    outputPath = AAudioConstants.DEFAULT_RECORDING_FILE,
                    description = "Emergency Fallback - Generic Recording"
                )
            )
        }
    }
}