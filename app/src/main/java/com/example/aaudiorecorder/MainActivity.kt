package com.example.aaudiorecorder

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.example.aaudiorecorder.config.AAudioConfig
import com.example.aaudiorecorder.recorder.AAudioRecorder

/**
 * AAudio录音器主界面
 * 
 * 使用说明:
 * 1. 确保设备支持AAudio API (Android 8.1+)
 * 2. 授予录音权限
 * 3. 选择录音配置
 * 4. 开始录音
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var audioRecorder: AAudioRecorder
    private lateinit var recordButton: Button
    private lateinit var stopButton: Button
    private lateinit var configButton: Button
    private lateinit var statusText: TextView
    private lateinit var recordingInfoText: TextView
    
    private var availableConfigs: List<AAudioConfig> = emptyList()
    private var currentConfig: AAudioConfig? = null

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        initializeViews()
        initializeAudioRecorder()
        loadConfigurations()
        checkPermissions()
    }
    
    private fun initializeViews() {
        recordButton = findViewById(R.id.recordButton)
        stopButton = findViewById(R.id.stopButton)
        configButton = findViewById(R.id.configButton)
        statusText = findViewById(R.id.statusTextView)
        recordingInfoText = findViewById(R.id.recordingInfoTextView)
        
        recordButton.setOnClickListener { startRecording() }
        stopButton.setOnClickListener { stopRecording() }
        configButton.setOnClickListener { showConfigDialog() }
        
        // 初始状态
        recordButton.isEnabled = true
        stopButton.isEnabled = false
        statusText.text = "准备录音"
    }
    
    private fun initializeAudioRecorder() {
        audioRecorder = AAudioRecorder()
        audioRecorder.setRecordingListener(object : AAudioRecorder.RecordingListener {
            override fun onRecordingStarted() {
                runOnUiThread {
                    recordButton.isEnabled = false
                    stopButton.isEnabled = true
                    configButton.isEnabled = false
                    statusText.text = "正在录音..."
                    updateRecordingInfo()
                }
            }
            
            override fun onRecordingStopped() {
                runOnUiThread {
                    recordButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "录音已停止"
                    updateRecordingInfo()
                }
            }
            
            @SuppressLint("SetTextI18n")
            override fun onRecordingError(error: String) {
                runOnUiThread {
                    recordButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "错误: $error"
                    Toast.makeText(this@MainActivity, error, Toast.LENGTH_SHORT).show()
                }
            }
        })
    }
    
    private fun loadConfigurations() {
        availableConfigs = try {
            AAudioConfig.loadConfigs(this)
        } catch (e: Exception) {
            Log.e(TAG, "加载配置失败", e)
            emptyList()
        }
        
        if (availableConfigs.isNotEmpty()) {
            currentConfig = availableConfigs[0]
            audioRecorder.setConfig(currentConfig!!)
            updateRecordingInfo()
            Log.i(TAG, "Loaded ${availableConfigs.size} recording configurations")
        } else {
            Log.e(TAG, "Failed to load recording configurations")
            statusText.text = "配置加载失败"
            recordButton.isEnabled = false
            configButton.isEnabled = false
        }
    }
    
    private fun checkPermissions() {
        if (!hasRecordPermissions()) {
            requestRecordPermissions()
        } else {
            onPermissionsGranted()
        }
    }
    
    private fun hasRecordPermissions(): Boolean {
        return ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
    }
    
    private fun requestRecordPermissions() {
        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), PERMISSION_REQUEST_CODE)
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            
            if (allGranted) {
                onPermissionsGranted()
            } else {
                Toast.makeText(this, "需要录音权限才能使用此应用", Toast.LENGTH_LONG).show()
                statusText.text = "权限被拒绝"
            }
        }
    }
    
    private fun onPermissionsGranted() {
        statusText.text = "准备录音"
        Log.i(TAG, "All permissions granted")
    }
    
    private fun startRecording() {
        if (audioRecorder.isRecording()) {
            Toast.makeText(this, "已在录音中", Toast.LENGTH_SHORT).show()
            return
        }
        
        if (!hasRecordPermissions()) {
            requestRecordPermissions()
            return
        }
        
        statusText.text = "准备录音..."
        if (audioRecorder.startRecording()) {
            Log.i(TAG, "Recording started")
        }
    }
    
    private fun stopRecording() {
        if (!audioRecorder.isRecording()) {
            Toast.makeText(this, "当前未在录音", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "正在停止..."
        if (audioRecorder.stopRecording()) {
            Log.i(TAG, "Recording stopped")
        }
    }
    
    @SuppressLint("SetTextI18n")
    private fun showConfigDialog() {
        if (availableConfigs.isEmpty()) {
            Toast.makeText(this, "没有可用的录音配置", Toast.LENGTH_SHORT).show()
            return
        }
        
        val configNames = availableConfigs.map { it.description }.toTypedArray()
        val currentIndex = availableConfigs.indexOf(currentConfig)
        
        AlertDialog.Builder(this)
            .setTitle("选择录音配置")
            .setSingleChoiceItems(configNames, currentIndex) { dialog, which ->
                currentConfig = availableConfigs[which]
                audioRecorder.setConfig(currentConfig!!)
                updateRecordingInfo()
                
                Toast.makeText(this, "已切换到: ${currentConfig!!.description}", Toast.LENGTH_SHORT).show()
                Log.i(TAG, "Config changed to: ${currentConfig!!.description}")
                
                dialog.dismiss()
            }
            .setNegativeButton("取消", null)
            .show()
    }
    
    @SuppressLint("SetTextI18n")
    private fun updateRecordingInfo() {
        currentConfig?.let { config ->
            val configInfo = "当前配置: ${config.description}\n" +
                    "源: ${config.inputPreset}\n" +
                    "模式: ${config.performanceMode} | ${config.sharingMode}\n" +
                    "文件: ${config.outputPath}"
            recordingInfoText.text = configInfo
        } ?: run {
            recordingInfoText.text = "录音信息"
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        audioRecorder.release()
        Log.i(TAG, "MainActivity destroyed")
    }
}