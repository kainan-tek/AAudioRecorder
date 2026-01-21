package com.example.aaudiorecorder.recorder

import android.content.Context
import android.util.Log
import com.example.aaudiorecorder.config.RecorderConfig

/**
 * AAudio录音器 - 参考AAudioPlayer的架构设计
 */
class AAudioRecorder(context: Context) {
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
    
    private var currentConfig: RecorderConfig = RecorderConfig()
    private var listener: RecordingListener? = null
    private var isRecording = false
    
    init {
        initializeNative()
    }
    
    /**
     * 设置录音监听器
     */
    fun setRecordingListener(listener: RecordingListener?) {
        this.listener = listener
    }
    
    /**
     * 设置录音配置
     */
    fun setConfig(config: RecorderConfig) {
        if (isRecording) {
            Log.w(TAG, "Cannot change config while recording")
            return
        }
        
        currentConfig = config
        Log.d(TAG, "Config updated: ${config.description}")
        
        // 获取实际的输出文件路径
        val actualOutputPath = config.getActualOutputPath()
        Log.d(TAG, "Using output path: $actualOutputPath")
        
        // 应用配置到native层
        setNativeConfig(
            config.getInputPresetValue(),
            config.sampleRate,
            config.channelCount,
            config.getFormatValue(),
            config.getPerformanceModeValue(),
            config.getSharingModeValue(),
            actualOutputPath
        )
    }

    /**
     * 开始录音
     */
    fun startRecording(): Boolean {
        if (isRecording) {
            Log.w(TAG, "Already recording")
            listener?.onRecordingError("已在录音中")
            return false
        }
        
        Log.d(TAG, "Starting recording with config: ${currentConfig.description}")
        
        val success = startNativeRecording()
        if (success) {
            isRecording = true
            listener?.onRecordingStarted()
            Log.i(TAG, "Recording started successfully")
        } else {
            listener?.onRecordingError("启动录音失败")
            Log.e(TAG, "Failed to start recording")
        }
        
        return success
    }
    
    /**
     * 停止录音
     */
    fun stopRecording(): Boolean {
        if (!isRecording) {
            Log.w(TAG, "Not recording")
            listener?.onRecordingError("当前未在录音")
            return false
        }
        
        Log.d(TAG, "Stopping recording")
        
        val success = stopNativeRecording()
        if (success) {
            isRecording = false
            listener?.onRecordingStopped()
            Log.i(TAG, "Recording stopped successfully")
        } else {
            listener?.onRecordingError("停止录音失败")
            Log.e(TAG, "Failed to stop recording")
        }
        
        return success
    }

    /**
     * 检查是否正在录音
     */
    fun isRecording(): Boolean {
        return isRecording
    }
    
    /**
     * 获取最后录音文件信息
     */
    fun getLastRecordingInfo(): String {
        return if (isRecording) {
            "录音中 - ${currentConfig.sampleRate}Hz, ${currentConfig.channelCount}声道, ${currentConfig.format}位"
        } else {
            val filePath = getLastRecordingPath()
            val fileSize = getLastRecordingSize()
            if (filePath.isNotEmpty()) {
                "最后录音: $filePath (${formatFileSize(fileSize)})"
            } else {
                "暂无录音文件"
            }
        }
    }
    
    /**
     * 释放资源
     */
    fun release() {
        if (isRecording) {
            stopRecording()
        }
        releaseNative()
        Log.d(TAG, "Resources released")
    }
    
    /**
     * 格式化文件大小
     */
    private fun formatFileSize(bytes: Long): String {
        return when {
            bytes < 1024 -> "${bytes}B"
            bytes < 1024 * 1024 -> "${bytes / 1024}KB"
            else -> "${bytes / (1024 * 1024)}MB"
        }
    }
    
    // Native方法声明
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
    private external fun getLastRecordingPath(): String
    private external fun getLastRecordingSize(): Long
    private external fun releaseNative()
    
    // 从Native层调用的回调方法
    @Suppress("unused")
    private fun onNativeRecordingStarted() {
        isRecording = true
        listener?.onRecordingStarted()
    }
    
    @Suppress("unused")
    private fun onNativeRecordingStopped() {
        isRecording = false
        listener?.onRecordingStopped()
    }
    
    @Suppress("unused")
    private fun onNativeRecordingError(error: String) {
        isRecording = false
        listener?.onRecordingError(error)
    }
}