package com.example.aaudiorecorder.common

/**
 * Common AAudio constants and utilities
 * 共享的AAudio常量和工具类
 */
object AAudioConstants {
    
    // Input presets
    const val INPUT_PRESET_GENERIC = "AAUDIO_INPUT_PRESET_GENERIC"
    const val INPUT_PRESET_CAMCORDER = "AAUDIO_INPUT_PRESET_CAMCORDER"
    const val INPUT_PRESET_VOICE_RECOGNITION = "AAUDIO_INPUT_PRESET_VOICE_RECOGNITION"
    const val INPUT_PRESET_VOICE_COMMUNICATION = "AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION"
    const val INPUT_PRESET_UNPROCESSED = "AAUDIO_INPUT_PRESET_UNPROCESSED"
    const val INPUT_PRESET_VOICE_PERFORMANCE = "AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE"
    const val INPUT_PRESET_SYSTEM_ECHO_REFERENCE = "AAUDIO_INPUT_PRESET_SYSTEM_ECHO_REFERENCE"
    const val INPUT_PRESET_SYSTEM_HOTWORD = "AAUDIO_INPUT_PRESET_SYSTEM_HOTWORD"
    
    // Performance modes
    const val PERFORMANCE_MODE_NONE = "AAUDIO_PERFORMANCE_MODE_NONE"
    const val PERFORMANCE_MODE_POWER_SAVING = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING"
    const val PERFORMANCE_MODE_LOW_LATENCY = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY"
    
    // Sharing modes
    const val SHARING_MODE_EXCLUSIVE = "AAUDIO_SHARING_MODE_EXCLUSIVE"
    const val SHARING_MODE_SHARED = "AAUDIO_SHARING_MODE_SHARED"
    
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
    const val DEFAULT_RECORDING_FILE = "/data/recorded_48k_1ch_16bit.wav"
    
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
     * Get input preset integer value
     */
    fun getInputPresetValue(inputPreset: String): Int {
        return when (inputPreset) {
            INPUT_PRESET_GENERIC -> AAudio.INPUT_PRESET_GENERIC
            INPUT_PRESET_CAMCORDER -> AAudio.INPUT_PRESET_CAMCORDER
            INPUT_PRESET_VOICE_RECOGNITION -> AAudio.INPUT_PRESET_VOICE_RECOGNITION
            INPUT_PRESET_VOICE_COMMUNICATION -> AAudio.INPUT_PRESET_VOICE_COMMUNICATION
            INPUT_PRESET_UNPROCESSED -> AAudio.INPUT_PRESET_UNPROCESSED
            INPUT_PRESET_VOICE_PERFORMANCE -> AAudio.INPUT_PRESET_VOICE_PERFORMANCE
            INPUT_PRESET_SYSTEM_ECHO_REFERENCE -> AAudio.INPUT_PRESET_SYSTEM_ECHO_REFERENCE
            INPUT_PRESET_SYSTEM_HOTWORD -> AAudio.INPUT_PRESET_SYSTEM_HOTWORD
            else -> AAudio.INPUT_PRESET_GENERIC // Default GENERIC
        }
    }
    
    /**
     * Get format integer value from bit depth
     */
    fun getFormatValueFromBitDepth(bitDepth: Int): Int {
        return when (bitDepth) {
            16 -> AAudio.FORMAT_PCM_I16
            24 -> AAudio.FORMAT_PCM_I24_PACKED
            32 -> AAudio.FORMAT_PCM_I32
            else -> AAudio.FORMAT_PCM_I16 // Default PCM_I16
        }
    }
    
    /**
     * Get performance mode integer value
     */
    fun getPerformanceModeValue(performanceMode: String): Int {
        return when (performanceMode) {
            PERFORMANCE_MODE_NONE -> AAudio.PERFORMANCE_MODE_NONE
            PERFORMANCE_MODE_POWER_SAVING -> AAudio.PERFORMANCE_MODE_POWER_SAVING
            PERFORMANCE_MODE_LOW_LATENCY -> AAudio.PERFORMANCE_MODE_LOW_LATENCY
            else -> AAudio.PERFORMANCE_MODE_LOW_LATENCY // Default LOW_LATENCY
        }
    }
    
    /**
     * Get sharing mode integer value
     */
    fun getSharingModeValue(sharingMode: String): Int {
        return when (sharingMode) {
            SHARING_MODE_EXCLUSIVE -> AAudio.SHARING_MODE_EXCLUSIVE
            SHARING_MODE_SHARED -> AAudio.SHARING_MODE_SHARED
            else -> AAudio.SHARING_MODE_SHARED // Default SHARED
        }
    }
    
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