package com.example.aaudiorecorder.recorder

import android.util.Log
import com.example.aaudiorecorder.config.AAudioConfig

/**
 * AAudio录音器 - 参考AAudioPlayer的架构设计
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
     * 设置录音监听器
     */
    fun setRecordingListener(listener: RecordingListener?) {
        this.listener = listener
    }
    
    /**
     * 设置录音配置
     */
    fun setConfig(config: AAudioConfig) {
        if (isRecording) {
            Log.w(TAG, "Cannot change config while recording")
            return
        }
        
        currentConfig = config
        Log.d(TAG, "Config updated: ${config.description}")
        Log.d(TAG, "Using output path: ${config.outputPath}")
        
        // 应用配置到native层，直接传递原始的outputPath
        setNativeConfig(
            config.getInputPresetValue(),
            config.sampleRate,
            config.channelCount,
            config.getFormatValue(),
            config.getPerformanceModeValue(),
            config.getSharingModeValue(),
            config.outputPath  // 直接使用原始路径，让C++代码处理文件名生成
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
        if (!success) {
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
        if (!success) {
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
     * 释放资源
     */
    fun release() {
        if (isRecording) {
            stopRecording()
        }
        releaseNative()
        Log.d(TAG, "Resources released")
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
    private external fun releaseNative()
    
    // 从Native层调用的回调方法
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