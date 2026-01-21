package com.example.aaudiorecorder

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.example.aaudiorecorder.config.RecorderConfig
import com.example.aaudiorecorder.recorder.AAudioRecorder

/**
 * AAudio录音器主界面 - 参考AAudioPlayer的架构设计
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
    
    private var availableConfigs: List<RecorderConfig> = emptyList()
    private var currentConfig: RecorderConfig? = null

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        initializeViews()
        initializeAudioRecorder()
        loadConfigs()
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
        audioRecorder = AAudioRecorder(this)
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
    
    private fun loadConfigs() {
        availableConfigs = RecorderConfig.loadConfigs(this)
        if (availableConfigs.isNotEmpty()) {
            currentConfig = availableConfigs[0]
            audioRecorder.setConfig(currentConfig!!)
            updateRecordingInfo()
        }
        Log.i(TAG, "Loaded ${availableConfigs.size} recording configurations")
    }
    
    private fun checkPermissions() {
        val permissions = mutableListOf<String>()
        
        // 录音权限
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) 
            != PackageManager.PERMISSION_GRANTED) {
            permissions.add(Manifest.permission.RECORD_AUDIO)
        }
        
        // 存储权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ 使用 READ_MEDIA_AUDIO
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_AUDIO) 
                != PackageManager.PERMISSION_GRANTED) {
                permissions.add(Manifest.permission.READ_MEDIA_AUDIO)
            }
        } else {
            // Android 12 及以下使用 READ_EXTERNAL_STORAGE
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) 
                != PackageManager.PERMISSION_GRANTED) {
                permissions.add(Manifest.permission.READ_EXTERNAL_STORAGE)
            }
        }
        
        // 写入存储权限 (Android 10 以下)
        if (permissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(this, permissions.toTypedArray(), PERMISSION_REQUEST_CODE)
        } else {
            onPermissionsGranted()
        }
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
        if (audioRecorder.startRecording()) {
            Log.i(TAG, "Recording started")
        }
    }
    
    private fun stopRecording() {
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
        val info = audioRecorder.getLastRecordingInfo()
        recordingInfoText.text = info
        
        currentConfig?.let { config ->
            val configInfo = "\n当前配置: ${config.description}"
            recordingInfoText.text = info + configInfo
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        audioRecorder.release()
        Log.i(TAG, "MainActivity destroyed")
    }
}