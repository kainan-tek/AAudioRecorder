package com.example.aaudiorecorder

import android.util.Log

/**
 * AudioRecorderManager - 管理AAudio录音功能的上层接口
 * 实现了录音按钮点击逻辑（多次点击无效）和停止录音按钮点击逻辑（多次点击无效）
 */
class AudioRecorderManager {
    private val logTag = "AudioRecorderManager"
    
    // 使用@Volatile确保多线程可见性
    @Volatile
    private var isRecording = false
    
    // 初始化录音器
    fun init(): Boolean {
        return nativeInit()
    }
    
    /**
     * 开始录音
     * 多次点击无效 - 如果正在录音，直接返回false
     */
    fun startRecording(): Boolean {
        // 多次点击无效 - 如果正在录音，直接返回false
        if (isRecording) {
            Log.w(logTag, "Already recording, cannot start again")
            return false
        }
        
        // 文件名由C++层内部管理，不传递任何参数
        val result = nativeStartRecording()
        if (result) {
            isRecording = true
            Log.i(logTag, "Started recording")
        }
        
        return result
    }
    
    /**
     * 停止录音
     * 多次点击无效 - 如果不在录音，直接返回false
     */
    fun stopRecording(): Boolean {
        // 多次点击无效 - 如果不在录音，直接返回false
        if (!isRecording) {
            Log.w(logTag, "Not recording, cannot stop")
            return false
        }
        
        val result = nativeStopRecording()
        if (result) {
            isRecording = false
            Log.i(logTag, "Stopped recording")
        }
        
        return result
    }
    
    // 释放资源
    fun release() {
        nativeRelease()
        isRecording = false
    }
    
    // 获取当前录音状态
    fun isRecording(): Boolean {
        return isRecording
    }
    
    // Native方法声明
    private external fun nativeInit(): Boolean
    private external fun nativeStartRecording(): Boolean
    private external fun nativeStopRecording(): Boolean
    private external fun nativeRelease()
    
    companion object {
        // 加载C++库
        init {
            System.loadLibrary("aaudiorecorder")
        }
    }
}
