package com.example.aaudiorecorder

import android.Manifest
import android.content.pm.PackageManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

/**
 * MainActivity - 应用的主界面
 * 实现了权限请求、录音按钮点击事件处理及状态管理
 * 确保多次连续点击录音和停止按钮无效
 */
class MainActivity : AppCompatActivity() {
    private val tag = "MainActivity"
    private val requestRecordAudioPermission = 1
    private lateinit var recordButton: Button
    private lateinit var stopButton: Button
    private lateinit var audioRecorderManager: AudioRecorderManager
    
    // 权限数组 - 包含录音和文件操作所需的所有权限
    private val permissions = arrayOf(
        Manifest.permission.RECORD_AUDIO,
        Manifest.permission.WRITE_EXTERNAL_STORAGE
    )
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        // 初始化UI组件
        recordButton = findViewById(R.id.button1)
        stopButton = findViewById(R.id.button2)
        
        // 初始化录音管理器
        audioRecorderManager = AudioRecorderManager()
        
        // 设置按钮点击事件
        recordButton.setOnClickListener {
            handleRecordButtonClick()
        }
        
        stopButton.setOnClickListener {
            handleStopButtonClick()
        }
        
        // 检查并请求权限
        checkAndRequestPermissions()
        
        // 初始化录音器
        audioRecorderManager.init()
        
        // 设置按钮初始状态：录音按钮可用，停止按钮不可用
        updateButtonStates(false)
    }
    
    /**
     * 处理录音按钮点击事件
     * 多次点击无效 - 如果正在录音，不执行任何操作
     */
    private fun handleRecordButtonClick() {
        // 权限检查
        if (!hasAllPermissions()) {
            Log.w(tag, "Permission not granted, cannot start recording")
            checkAndRequestPermissions()
            return
        }
        
        // AudioRecorderManager内部会处理多次点击无效的逻辑
        val success = audioRecorderManager.startRecording()
        
        if (success) {
            // 更新UI状态
            updateButtonStates(true)
            Log.i(tag, "Recording started successfully")
        } else {
            Log.w(tag, "Failed to start recording or already recording")
        }
    }
    
    /**
     * 处理停止录音按钮点击事件
     * 多次点击无效 - 如果不在录音，不执行任何操作
     */
    private fun handleStopButtonClick() {
        // AudioRecorderManager内部会处理多次点击无效的逻辑
        val success = audioRecorderManager.stopRecording()
        
        if (success) {
            // 更新UI状态
            updateButtonStates(false)
            Log.i(tag, "Recording stopped successfully")
        } else {
            Log.w(tag, "Failed to stop recording or not recording")
        }
    }
    
    /**
     * 检查并请求权限
     */
    private fun checkAndRequestPermissions() {
        if (!hasAllPermissions()) {
            ActivityCompat.requestPermissions(this, permissions, requestRecordAudioPermission)
        }
    }
    
    /**
     * 检查是否拥有所有必要的权限
     */
    private fun hasAllPermissions(): Boolean {
        for (permission in permissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                return false
            }
        }
        return true
    }
    
    /**
     * 更新按钮状态
     */
    private fun updateButtonStates(isRecording: Boolean) {
        runOnUiThread {
            recordButton.isEnabled = !isRecording
            stopButton.isEnabled = isRecording
        }
    }
    
    /**
     * 处理权限请求结果
     */
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == requestRecordAudioPermission) {
            // 检查所有权限是否都已授予
            var allGranted = true
            for (i in grantResults.indices) {
                if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false
                    Log.w(tag, "Permission denied: ${permissions[i]}")
                } else {
                    Log.i(tag, "Permission granted: ${permissions[i]}")
                }
            }
            
            if (!allGranted) {
                Log.w(tag, "Some permissions were denied, recording functionality may be limited")
            }
        }
    }
    
    /**
     * 处理应用暂停事件
     */
    override fun onPause() {
        super.onPause()
        
        // 如果正在录音，停止录音
        if (audioRecorderManager.isRecording()) {
            audioRecorderManager.stopRecording()
            updateButtonStates(false)
        }
    }
    
    /**
     * 处理应用销毁事件
     */
    override fun onDestroy() {
        super.onDestroy()
        
        // 释放录音资源
        audioRecorderManager.release()
    }
}