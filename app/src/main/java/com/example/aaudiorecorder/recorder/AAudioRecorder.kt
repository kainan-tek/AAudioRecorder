package com.example.aaudiorecorder.recorder

import android.util.Log
import com.example.aaudiorecorder.config.AAudioConfig
import com.example.aaudiorecorder.common.AAudioConstants

/**
 * AAudio Recorder - enhanced with better error handling
 */
class AAudioRecorder {
    companion object {
        private const val TAG = "AAudioRecorder"
        
        init {
            try {
                System.loadLibrary("aaudiorecorder")
                Log.d(TAG, "Native library loaded")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
            }
        }
    }
    
    interface RecordingListener {
        fun onRecordingStarted()
        fun onRecordingStopped()
        fun onRecordingError(error: String)
    }
    
    private var currentConfig: AAudioConfig = AAudioConfig()
    private var listener: RecordingListener? = null
    private var isRecording = false
    
    init {
        initializeNative()
    }
    
    /**
     * Set recording listener
     */
    fun setRecordingListener(listener: RecordingListener?) {
        this.listener = listener
    }
    
    fun setAudioConfig(config: AAudioConfig) {
        if (isRecording) {
            Log.w(TAG, "Cannot change config while recording")
            return
        }
        
        currentConfig = config
        Log.i(TAG, "Configuration updated: ${currentConfig.description}")
        
        // Apply configuration to native layer
        setNativeConfig(
            AAudioConstants.getInputPreset(currentConfig.inputPreset),
            currentConfig.sampleRate,
            currentConfig.channelCount,
            AAudioConstants.getFormatFromBitDepth(currentConfig.format),
            AAudioConstants.getPerformanceMode(currentConfig.performanceMode),
            AAudioConstants.getSharingMode(currentConfig.sharingMode),
            currentConfig.outputPath
        )
    }

    /**
     * Start recording
     */
    fun startRecording(): Boolean {
        if (isRecording) {
            Log.w(TAG, "Already recording")
            listener?.onRecordingError("Already recording")
            return false
        }
        
        // Validate configuration before starting
        if (!AAudioConstants.isValidSampleRate(currentConfig.sampleRate)) {
            val error = "Invalid sample rate: ${currentConfig.sampleRate}"
            Log.e(TAG, error)
            listener?.onRecordingError(error)
            return false
        }
        
        if (!AAudioConstants.isValidChannelCount(currentConfig.channelCount)) {
            val error = "Invalid channel count: ${currentConfig.channelCount}"
            Log.e(TAG, error)
            listener?.onRecordingError(error)
            return false
        }
        
        // Validate output path
        if (currentConfig.outputPath.isNotBlank() && !currentConfig.outputPath.endsWith(".wav")) {
            val error = "Invalid output path: must be empty or end with .wav"
            Log.e(TAG, error)
            listener?.onRecordingError(error)
            return false
        }
        
        Log.d(TAG, "Starting recording with config: ${currentConfig.description}")
        
        val success = startNativeRecording()
        if (!success) {
            val error = "Failed to start recording - check permissions and configuration"
            listener?.onRecordingError(error)
            Log.e(TAG, error)
        }
        
        return success
    }
    
    fun stopRecording(): Boolean {
        if (!isRecording) {
            Log.w(TAG, "Not currently recording")
            listener?.onRecordingError("Not currently recording")
            return false
        }
        
        Log.d(TAG, "Stopping recording")
        
        val success = stopNativeRecording()
        if (!success) {
            val error = "Failed to stop recording"
            listener?.onRecordingError(error)
            Log.e(TAG, error)
        }
        
        return success
    }

    /**
     * Check if currently recording
     */
    fun isRecording(): Boolean {
        return isRecording
    }
    
    /**
     * Release resources
     */
    fun release() {
        if (isRecording) {
            stopRecording()
        }
        try {
            releaseNative()
        } catch (e: Exception) {
            Log.e(TAG, "Error releasing native resources", e)
        }
        Log.d(TAG, "AAudioRecorder resources released")
    }
    
    // Native method declarations
    private external fun initializeNative(): Boolean
    private external fun setNativeConfig(
        inputPreset: Int,
        sampleRate: Int,
        channelCount: Int,
        format: Int,
        performanceMode: Int,
        sharingMode: Int,
        outputPath: String
    ): Boolean
    private external fun startNativeRecording(): Boolean
    private external fun stopNativeRecording(): Boolean
    private external fun releaseNative()
    
    // Callback methods called from Native layer
    @Suppress("unused")
    private fun onNativeRecordingStarted() {
        isRecording = true
        listener?.onRecordingStarted()
        Log.i(TAG, "Recording started successfully")
    }
    
    @Suppress("unused")
    private fun onNativeRecordingStopped() {
        isRecording = false
        listener?.onRecordingStopped()
        Log.i(TAG, "Recording stopped successfully")
    }
    
    @Suppress("unused")
    private fun onNativeRecordingError(error: String) {
        isRecording = false
        listener?.onRecordingError(error)
        Log.e(TAG, "Recording error: $error")
    }
}