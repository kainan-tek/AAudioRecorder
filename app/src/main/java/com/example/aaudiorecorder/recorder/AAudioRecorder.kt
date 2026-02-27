package com.example.aaudiorecorder.recorder

import android.content.Context
import android.util.Log
import com.example.aaudiorecorder.config.AAudioConfig
import com.example.aaudiorecorder.common.AAudioConstants

/**
 * AAudio Recorder - enhanced with better error handling
 */
class AAudioRecorder(private val context: Context? = null) {
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
     * Set audio configuration
     * Note: Output path will be generated at recording start if empty
     */
    fun setAudioConfig(config: AAudioConfig) {
        if (isRecording) {
            Log.w(TAG, "Cannot change config while recording")
            return
        }

        currentConfig = config
        Log.i(TAG, "Configuration updated: ${currentConfig.description}")

        // Validate output path (empty is valid, means use default path)
        if (currentConfig.outputPath.isNotBlank() && !currentConfig.outputPath.endsWith(".wav")) {
            Log.e(TAG, "Invalid output path: must be empty or end with .wav")
            return
        }

        // If outputPath is empty, pass only the default directory path to native layer,
        // Native layer will generate timestamped filename at recording start
        val outputPath = currentConfig.outputPath.ifBlank {
            getDefaultDirectory()
        }

        // Apply configuration to native layer immediately
        setNativeConfig(
            AAudioConstants.getInputPreset(currentConfig.inputPreset),
            currentConfig.sampleRate,
            currentConfig.channelCount,
            AAudioConstants.getFormatFromBitDepth(currentConfig.format),
            AAudioConstants.getPerformanceMode(currentConfig.performanceMode),
            AAudioConstants.getSharingMode(currentConfig.sharingMode),
            outputPath
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

        Log.d(TAG, "Starting recording with config: ${currentConfig.description}")

        return startNativeRecording()
    }

    /**
     * Get default directory path for recording files
     */
    private fun getDefaultDirectory(): String {
        val ctx = context
            ?: throw IllegalStateException("Context is required for default file path generation")
        return ctx.getExternalFilesDir(null)?.absolutePath
            ?: throw IllegalStateException("Failed to get external files directory")
    }

    fun stopRecording(): Boolean {
        if (!isRecording) {
            Log.w(TAG, "Not currently recording")
            listener?.onRecordingError("Not currently recording")
            return false
        }

        Log.d(TAG, "Stopping recording")

        return stopNativeRecording()
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
        outputPath: String,
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
