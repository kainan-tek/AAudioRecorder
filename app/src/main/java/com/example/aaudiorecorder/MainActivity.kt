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
 * AAudio Recorder Main Activity
 * 
 * Usage Instructions:
 * 1. Ensure device supports AAudio API (Android 8.1+)
 * 2. Grant recording permissions
 * 3. Select recording configuration
 * 4. Start recording
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
    
    @SuppressLint("SetTextI18n")
    private fun initializeViews() {
        recordButton = findViewById(R.id.recordButton)
        stopButton = findViewById(R.id.stopButton)
        configButton = findViewById(R.id.configButton)
        statusText = findViewById(R.id.statusTextView)
        recordingInfoText = findViewById(R.id.recordingInfoTextView)
        
        recordButton.setOnClickListener { startRecording() }
        stopButton.setOnClickListener { stopRecording() }
        configButton.setOnClickListener { showConfigDialog() }
        
        // Initial state
        recordButton.isEnabled = true
        stopButton.isEnabled = false
        statusText.text = "Ready to record"
    }
    
    private fun initializeAudioRecorder() {
        audioRecorder = AAudioRecorder()
        audioRecorder.setRecordingListener(object : AAudioRecorder.RecordingListener {
            @SuppressLint("SetTextI18n")
            override fun onRecordingStarted() {
                runOnUiThread {
                    recordButton.isEnabled = false
                    stopButton.isEnabled = true
                    configButton.isEnabled = false
                    statusText.text = "Recording..."
                    updateRecordingInfo()
                }
            }
            
            @SuppressLint("SetTextI18n")
            override fun onRecordingStopped() {
                runOnUiThread {
                    recordButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "Recording stopped"
                    updateRecordingInfo()
                }
            }
            
            @SuppressLint("SetTextI18n")
            override fun onRecordingError(error: String) {
                runOnUiThread {
                    recordButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "Error: $error"
                    Toast.makeText(this@MainActivity, error, Toast.LENGTH_SHORT).show()
                }
            }
        })
    }
    
    @SuppressLint("SetTextI18n")
    private fun loadConfigurations() {
        availableConfigs = try {
            AAudioConfig.loadConfigs(this)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load configurations", e)
            emptyList()
        }
        
        if (availableConfigs.isNotEmpty()) {
            currentConfig = availableConfigs[0]
            audioRecorder.setConfig(currentConfig!!)
            updateRecordingInfo()
            Log.i(TAG, "Loaded ${availableConfigs.size} recording configurations")
        } else {
            Log.e(TAG, "Failed to load recording configurations")
            statusText.text = "Configuration load failed"
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
    
    @SuppressLint("SetTextI18n")
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
                Toast.makeText(this, "Recording permission required to use this app", Toast.LENGTH_LONG).show()
                statusText.text = "Permission denied"
            }
        }
    }
    
    @SuppressLint("SetTextI18n")
    private fun onPermissionsGranted() {
        statusText.text = "Ready to record"
        Log.i(TAG, "All permissions granted")
    }
    
    @SuppressLint("SetTextI18n")
    private fun startRecording() {
        if (audioRecorder.isRecording()) {
            Toast.makeText(this, "Already recording", Toast.LENGTH_SHORT).show()
            return
        }
        
        if (!hasRecordPermissions()) {
            requestRecordPermissions()
            return
        }
        
        statusText.text = "Preparing to record..."
        if (audioRecorder.startRecording()) {
            Log.i(TAG, "Recording started")
        }
    }
    
    @SuppressLint("SetTextI18n")
    private fun stopRecording() {
        if (!audioRecorder.isRecording()) {
            Toast.makeText(this, "Not currently recording", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "Stopping..."
        if (audioRecorder.stopRecording()) {
            Log.i(TAG, "Recording stopped")
        }
    }
    
    @SuppressLint("SetTextI18n")
    private fun showConfigDialog() {
        if (availableConfigs.isEmpty()) {
            Toast.makeText(this, "No available recording configurations", Toast.LENGTH_SHORT).show()
            return
        }
        
        val configNames = availableConfigs.map { it.description }.toTypedArray()
        val currentIndex = availableConfigs.indexOf(currentConfig)
        
        AlertDialog.Builder(this)
            .setTitle("Select Recording Configuration")
            .setSingleChoiceItems(configNames, currentIndex) { dialog, which ->
                currentConfig = availableConfigs[which]
                audioRecorder.setConfig(currentConfig!!)
                updateRecordingInfo()
                
                Toast.makeText(this, "Switched to: ${currentConfig!!.description}", Toast.LENGTH_SHORT).show()
                Log.i(TAG, "Config changed to: ${currentConfig!!.description}")
                
                dialog.dismiss()
            }
            .setNegativeButton("Cancel", null)
            .show()
    }
    
    @SuppressLint("SetTextI18n")
    private fun updateRecordingInfo() {
        currentConfig?.let { config ->
            val configInfo = "Current Config: ${config.description}\n" +
                    "Source: ${config.inputPreset}\n" +
                    "Mode: ${config.performanceMode} | ${config.sharingMode}\n" +
                    "File: ${config.outputPath}"
            recordingInfoText.text = configInfo
        } ?: run {
            recordingInfoText.text = "Recording Info"
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        audioRecorder.release()
        Log.i(TAG, "MainActivity destroyed")
    }
}