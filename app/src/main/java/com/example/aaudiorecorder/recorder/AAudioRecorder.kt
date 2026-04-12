package com.example.aaudiorecorder.recorder

import android.content.Context
import android.util.Log
import com.example.aaudiorecorder.config.AAudioConfig
import com.example.aaudiorecorder.common.AAudioConstants

/**
 * Recording state enumeration
 */
enum class RecordingState {
    IDLE, RECORDING, ERROR
}

/**
 * Audio recorder using AAudio API
 */
class AAudioRecorder(private val context: Context) {
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

    @Volatile
    private var state = RecordingState.IDLE

    init {
        initializeNative()
    }

    fun setRecordingListener(listener: RecordingListener?) {
        this.listener = listener
    }

    fun setAudioConfig(config: AAudioConfig) {
        if (state == RecordingState.RECORDING) {
            Log.w(TAG, "Cannot change configuration while recording")
            return
        }

        // Validate output path before applying (empty is valid, means use default path)
        if (config.audioFilePath.isNotBlank() && !config.audioFilePath.endsWith(".wav")) {
            Log.e(TAG, "Invalid output path: must be empty or end with .wav")
            return
        }

        currentConfig = config
        Log.i(TAG, "Configuration updated: ${config.description}")

        // If audioFilePath is empty, pass only the default directory path to native layer,
        // Native layer will generate timestamped filename at recording start
        val audioFilePath = currentConfig.audioFilePath.ifBlank {
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
            audioFilePath
        )
    }

    fun startRecording(): Boolean {
        if (state == RecordingState.RECORDING) {
            Log.w(TAG, "Already recording")
            listener?.onRecordingError("Already recording")
            return false
        }
        if (state == RecordingState.ERROR) {
            state = RecordingState.IDLE
        }

        // Validate configuration before starting
        if (!AAudioConstants.isValidSampleRate(currentConfig.sampleRate)) {
            val error =
                "${AAudioConstants.ErrorTypes.PARAM} Invalid sample rate: ${currentConfig.sampleRate}"
            Log.e(TAG, error)
            listener?.onRecordingError(error)
            return false
        }

        if (!AAudioConstants.isValidChannelCount(currentConfig.channelCount)) {
            val error =
                "${AAudioConstants.ErrorTypes.PARAM} Invalid channel count: ${currentConfig.channelCount}"
            Log.e(TAG, error)
            listener?.onRecordingError(error)
            return false
        }

        if (!AAudioConstants.isValidFormat(currentConfig.format)) {
            val error =
                "${AAudioConstants.ErrorTypes.PARAM} Invalid bit depth: ${currentConfig.format}"
            Log.e(TAG, error)
            listener?.onRecordingError(error)
            return false
        }

        Log.d(TAG, "Starting recording with config: ${currentConfig.description}")

        return startNativeRecording()
    }

    private fun getDefaultDirectory(): String {
        val ctx = context
        return ctx.getExternalFilesDir(null)?.absolutePath
            ?: throw IllegalStateException("Failed to get external files directory")
    }

    fun stopRecording() {
        if (state != RecordingState.RECORDING) {
            return
        }

        Log.d(TAG, "Stopping recording")

        stopNativeRecording()
    }

    fun isRecording(): Boolean = state == RecordingState.RECORDING

    fun release() {
        if (state == RecordingState.RECORDING) {
            stopRecording()
        }
        listener = null
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
        audioFilePath: String,
    ): Boolean

    private external fun startNativeRecording(): Boolean
    private external fun stopNativeRecording(): Boolean
    private external fun releaseNative()

    // Callback methods called from Native layer
    @Suppress("unused")
    private fun onNativeRecordingStarted() {
        state = RecordingState.RECORDING
        listener?.onRecordingStarted()
        Log.i(TAG, "Recording started successfully")
    }

    @Suppress("unused")
    private fun onNativeRecordingStopped() {
        state = RecordingState.IDLE
        listener?.onRecordingStopped()
        Log.i(TAG, "Recording stopped")
    }

    @Suppress("unused")
    private fun onNativeRecordingError(error: String) {
        state = RecordingState.ERROR
        listener?.onRecordingError(error)
        Log.e(TAG, "Recording error: $error")
    }
}
