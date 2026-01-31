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
        
        updateButtonStates(false)
        statusText.text = "Ready to record"
    }
    
    private fun initializeAudioRecorder() {
        audioRecorder = AAudioRecorder()
        audioRecorder.setRecordingListener(object : AAudioRecorder.RecordingListener {
            @SuppressLint("SetTextI18n")
            override fun onRecordingStarted() {
                runOnUiThread {
                    updateButtonStates(true)
                    statusText.text = "Recording..."
                    updateRecordingInfo()
                }
            }
            
            @SuppressLint("SetTextI18n")
            override fun onRecordingStopped() {
                runOnUiThread {
                    updateButtonStates(false)
                    statusText.text = "Recording stopped"
                    updateRecordingInfo()
                }
            }
            
            @SuppressLint("SetTextI18n")
            override fun onRecordingError(error: String) {
                runOnUiThread {
                    updateButtonStates(false)
                    statusText.text = "Error: $error"
                    Toast.makeText(this@MainActivity, error, Toast.LENGTH_SHORT).show()
                }
            }
        })
    }

    private fun updateButtonStates(isActive: Boolean) {
        recordButton.isEnabled = !isActive
        stopButton.isEnabled = isActive
        configButton.isEnabled = !isActive
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
            audioRecorder.setAudioConfig(currentConfig!!)
            updateRecordingInfo()
            Log.i(TAG, "Loaded ${availableConfigs.size} configurations")
        } else {
            statusText.text = "Configuration load failed"
            recordButton.isEnabled = false
            configButton.isEnabled = false
        }
    }
    
    private fun checkPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), PERMISSION_REQUEST_CODE)
        }
    }
    
    @SuppressLint("SetTextI18n")
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                Log.i(TAG, "Permissions granted")
            } else {
                Toast.makeText(this, "Recording permission required to use this app", Toast.LENGTH_LONG).show()
                statusText.text = "Permission denied"
            }
        }
    }
    
    @SuppressLint("SetTextI18n")
    private fun startRecording() {
        if (audioRecorder.isRecording()) {
            Toast.makeText(this, "Already recording", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "Preparing to record..."
        audioRecorder.startRecording()
    }
    
    @SuppressLint("SetTextI18n")
    private fun stopRecording() {
        if (!audioRecorder.isRecording()) {
            Toast.makeText(this, "Not currently recording", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "Stopping..."
        audioRecorder.stopRecording()
    }
    
    @SuppressLint("SetTextI18n")
    private fun showConfigDialog() {
        if (availableConfigs.isEmpty()) {
            Toast.makeText(this, "No available configurations", Toast.LENGTH_SHORT).show()
            return
        }
        
        val configNames = availableConfigs.map { it.description }.toTypedArray()
        val currentIndex = availableConfigs.indexOf(currentConfig)
        
        AlertDialog.Builder(this)
            .setTitle("Select Configuration")
            .setSingleChoiceItems(configNames, currentIndex) { dialog, which ->
                currentConfig = availableConfigs[which]
                audioRecorder.setAudioConfig(currentConfig!!)
                updateRecordingInfo()
                Toast.makeText(this, "Switched to: ${currentConfig!!.description}", Toast.LENGTH_SHORT).show()
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
    }
}