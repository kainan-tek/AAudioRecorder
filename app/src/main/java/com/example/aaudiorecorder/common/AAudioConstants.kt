package com.example.aaudiorecorder.common

/**
 * Common AAudio constants and utilities
 */
object AAudioConstants {
    
    // Audio formats - bit depth constants for reference
    const val FORMAT_16_BIT = 16
    const val FORMAT_24_BIT = 24
    const val FORMAT_32_BIT = 32
    
    // Sample rate validation range
    const val MIN_SAMPLE_RATE = 8000
    const val MAX_SAMPLE_RATE = 192000
    
    // Channel count validation range
    const val MIN_CHANNEL_COUNT = 1
    const val MAX_CHANNEL_COUNT = 16
    
    // Configuration file paths
    const val CONFIG_FILE_PATH = "/data/aaudio_recorder_configs.json"
    const val ASSETS_CONFIG_FILE = "aaudio_recorder_configs.json"
    
    // Default audio file paths
    const val DEFAULT_RECORD_FILE = "/data/recorded_48k_1ch_16bit.wav"
    
    /**
     * AAudio native constants (matching NDK definitions)
     */
    object AAudio {
        // Input preset values
        const val INPUT_PRESET_GENERIC = 1
        const val INPUT_PRESET_CAMCORDER = 5
        const val INPUT_PRESET_VOICE_RECOGNITION = 6
        const val INPUT_PRESET_VOICE_COMMUNICATION = 7
        const val INPUT_PRESET_UNPROCESSED = 9
        const val INPUT_PRESET_VOICE_PERFORMANCE = 10
        const val INPUT_PRESET_SYSTEM_ECHO_REFERENCE = 1997
        const val INPUT_PRESET_SYSTEM_HOTWORD = 1999
        
        // Format values
        const val FORMAT_PCM_I16 = 1
        const val FORMAT_PCM_I24_PACKED = 3
        const val FORMAT_PCM_I32 = 4
        
        // Performance mode values
        const val PERFORMANCE_MODE_NONE = 10
        const val PERFORMANCE_MODE_POWER_SAVING = 11
        const val PERFORMANCE_MODE_LOW_LATENCY = 12
        
        // Sharing mode values
        const val SHARING_MODE_EXCLUSIVE = 0
        const val SHARING_MODE_SHARED = 1
    }
    
    /**
     * Input preset constants mapping
     */
    object InputPreset {
        val MAP = mapOf(
            AAudio.INPUT_PRESET_GENERIC to "AAUDIO_INPUT_PRESET_GENERIC",
            AAudio.INPUT_PRESET_CAMCORDER to "AAUDIO_INPUT_PRESET_CAMCORDER",
            AAudio.INPUT_PRESET_VOICE_RECOGNITION to "AAUDIO_INPUT_PRESET_VOICE_RECOGNITION",
            AAudio.INPUT_PRESET_VOICE_COMMUNICATION to "AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION",
            AAudio.INPUT_PRESET_UNPROCESSED to "AAUDIO_INPUT_PRESET_UNPROCESSED",
            AAudio.INPUT_PRESET_VOICE_PERFORMANCE to "AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE",
            AAudio.INPUT_PRESET_SYSTEM_ECHO_REFERENCE to "AAUDIO_INPUT_PRESET_SYSTEM_ECHO_REFERENCE",
            AAudio.INPUT_PRESET_SYSTEM_HOTWORD to "AAUDIO_INPUT_PRESET_SYSTEM_HOTWORD"
        )
    }
    
    /**
     * Performance mode constants mapping
     */
    object PerformanceMode {
        val MAP = mapOf(
            AAudio.PERFORMANCE_MODE_NONE to "AAUDIO_PERFORMANCE_MODE_NONE",
            AAudio.PERFORMANCE_MODE_POWER_SAVING to "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
            AAudio.PERFORMANCE_MODE_LOW_LATENCY to "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY"
        )
    }
    
    /**
     * Sharing mode constants mapping
     */
    object SharingMode {
        val MAP = mapOf(
            AAudio.SHARING_MODE_EXCLUSIVE to "AAUDIO_SHARING_MODE_EXCLUSIVE",
            AAudio.SHARING_MODE_SHARED to "AAUDIO_SHARING_MODE_SHARED"
        )
    }

    /**
     * Get format integer value from bit depth
     */
    fun getFormatFromBitDepth(bitDepth: Int): Int {
        return when (bitDepth) {
            16 -> AAudio.FORMAT_PCM_I16
            24 -> AAudio.FORMAT_PCM_I24_PACKED
            32 -> AAudio.FORMAT_PCM_I32
            else -> AAudio.FORMAT_PCM_I16 // Default PCM_I16
        }
    }
    
    /**
     * Generic enum value parser with error handling
     */
    private fun parseEnumValue(map: Map<Int, String>, value: String, default: Int, typeName: String = ""): Int {
        val result = map.entries.find { it.value == value }?.key ?: default
        if (result == default && value.isNotEmpty()) {
            android.util.Log.w("AAudioConstants", "Unknown $typeName value: $value, using default")
        }
        return result
    }
    
    /**
     * Get input preset integer value
     */
    fun getInputPreset(inputPreset: String): Int =
        parseEnumValue(InputPreset.MAP, inputPreset, AAudio.INPUT_PRESET_GENERIC, "InputPreset")

    /**
     * Get performance mode integer value
     */
    fun getPerformanceMode(performanceMode: String): Int =
        parseEnumValue(PerformanceMode.MAP, performanceMode, AAudio.PERFORMANCE_MODE_LOW_LATENCY, "PerformanceMode")
    
    /**
     * Get sharing mode integer value
     */
    fun getSharingMode(sharingMode: String): Int =
        parseEnumValue(SharingMode.MAP, sharingMode, AAudio.SHARING_MODE_SHARED, "SharingMode")
    
    /**
     * Validate sample rate
     */
    fun isValidSampleRate(sampleRate: Int): Boolean {
        return sampleRate in MIN_SAMPLE_RATE..MAX_SAMPLE_RATE
    }
    
    /**
     * Validate channel count
     */
    fun isValidChannelCount(channelCount: Int): Boolean {
        return channelCount in MIN_CHANNEL_COUNT..MAX_CHANNEL_COUNT
    }
    
    /**
     * Validate format bit depth
     */
    fun isValidFormat(bitDepth: Int): Boolean {
        return bitDepth in listOf(FORMAT_16_BIT, FORMAT_24_BIT, FORMAT_32_BIT)
    }
}