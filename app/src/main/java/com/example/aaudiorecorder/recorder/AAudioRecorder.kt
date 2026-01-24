package com.example.aaudiorecorder.recorder

import android.util.Log
import com.example.aaudiorecorder.config.AAudioConfig

/**
 * AAudio Recorder - based on AAudioPlayer architecture design
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
    
    /**
     * Set recording configuration
     */
    fun setConfig(config: AAudioConfig) {
        if (isRecording) {
            Log.w(TAG, "Cannot change config while recording")
            return
        }
        
        currentConfig = config
        Log.d(TAG, "Config updated: ${config.description}")
        Log.d(TAG, "Using output path: ${config.outputPath}")
        
        // Apply configuration to native layer, directly pass original outputPath
        setNativeConfig(
            config.getInputPresetValue(),
            config.sampleRate,
            config.channelCount,
            config.getFormatValue(),
            config.getPerformanceModeValue(),
            config.getSharingModeValue(),
            config.outputPath  // Use original path directly, let C++ code handle filename generation
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
        
        Log.d(TAG, "Starting recording with config: ${currentConfig.description}")
        
        val success = startNativeRecording()
        if (!success) {
            listener?.onRecordingError("Failed to start recording")
            Log.e(TAG, "Failed to start recording")
        }
        
        return success
    }
    
    /**
     * Stop recording
     */
    fun stopRecording(): Boolean {
        if (!isRecording) {
            Log.w(TAG, "Not recording")
            listener?.onRecordingError("Not currently recording")
            return false
        }
        
        Log.d(TAG, "Stopping recording")
        
        val success = stopNativeRecording()
        if (!success) {
            listener?.onRecordingError("Failed to stop recording")
            Log.e(TAG, "Failed to stop recording")
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
        releaseNative()
        Log.d(TAG, "Resources released")
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