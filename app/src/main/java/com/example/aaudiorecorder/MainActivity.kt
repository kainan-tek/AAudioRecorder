package com.example.aaudiorecorder

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.Spinner
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
 * 3. Select recording configuration from dropdown
 * 4. Start recording
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var audioRecorder: AAudioRecorder
    private lateinit var recordButton: Button
    private lateinit var stopButton: Button
    private lateinit var configSpinner: Spinner
    private lateinit var statusText: TextView
    private lateinit var recordingInfoText: TextView
    
    private var availableConfigs: List<AAudioConfig> = emptyList()
    private var currentConfig: AAudioConfig? = null
    private var isSpinnerInitialized = false

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Hide title bar
        supportActionBar?.hide()
        setContentView(R.layout.activity_main)
        
        initViews()
        initializeAudioRecorder()
        loadConfigurations()
        if (!hasAudioPermission()) requestAudioPermission()
    }

    private fun initViews() {
        recordButton = findViewById(R.id.recordButton)
        stopButton = findViewById(R.id.stopButton)
        configSpinner = findViewById(R.id.configSpinner)
        statusText = findViewById(R.id.statusTextView)
        recordingInfoText = findViewById(R.id.recordingInfoTextView)
        
        recordButton.setOnClickListener {
            if (!hasAudioPermission()) {
                requestAudioPermission()
                return@setOnClickListener
            }
            startRecording()
        }
        
        stopButton.setOnClickListener {
            stopRecording()
        }
        
        updateButtonStates(false)
    }
    
    private fun initializeAudioRecorder() {
        audioRecorder = AAudioRecorder()
        audioRecorder.setRecordingListener(object : AAudioRecorder.RecordingListener {
            @SuppressLint("SetTextI18n")
            override fun onRecordingStarted() {
                runOnUiThread {
                    updateButtonStates(true)
                    statusText.text = "Recording in progress"
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
                    Toast.makeText(this@MainActivity, "Error: $error", Toast.LENGTH_SHORT).show()
                }
            }
        })
    }

    private fun updateButtonStates(isActive: Boolean) {
        recordButton.isEnabled = !isActive
        stopButton.isEnabled = isActive
        configSpinner.isEnabled = !isActive
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
            setupConfigSpinner()
            updateRecordingInfo()
            statusText.text = "Ready to record"
            Log.i(TAG, "Loaded ${availableConfigs.size} recording configurations")
        } else {
            statusText.text = "Recording configuration load failed"
            recordButton.isEnabled = false
        }
    }
    
    /**
     * Setup configuration spinner
     */
    private fun setupConfigSpinner() {
        val configs = availableConfigs
        Log.d(TAG, "Setting up recording config spinner with ${configs.size} configurations")
        
        if (configs.isEmpty()) {
            Log.w(TAG, "No recording configurations available for spinner")
            return
        }
        
        val configNames = configs.map { it.description }
            Log.d(TAG, "Recording config names: $configNames")
        
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, configNames)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        configSpinner.adapter = adapter
        
        // Set initial selection
        currentConfig?.let {
            val index = configs.indexOfFirst { config -> config.description == it.description }
            if (index >= 0) {
                configSpinner.setSelection(index)
                Log.d(TAG, "Set initial recording spinner selection to index $index: ${it.description}")
            }
        }
        
        configSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (!isSpinnerInitialized) {
                    isSpinnerInitialized = true
                    Log.d(TAG, "Recording spinner initialized, skipping first selection")
                    return
                }
                
                val selectedConfig = configs[position]
                Log.d(TAG, "Recording config selected: ${selectedConfig.description}")
                currentConfig = selectedConfig
                audioRecorder.setAudioConfig(selectedConfig)
                updateRecordingInfo()
                showToast("Switched to recording config: ${selectedConfig.description}")
            }
            
            override fun onNothingSelected(parent: AdapterView<*>?) {
                Log.d(TAG, "Nothing selected in recording spinner")
            }
        }
        
        // Add long press listener to reload configurations
        configSpinner.setOnLongClickListener {
            Log.d(TAG, "Long press detected on recording spinner")
            reloadConfigurations()
            true
        }
    }
    
    /**
     * Reload configuration file
     */
    private fun reloadConfigurations() {
        try {
            loadConfigurations()
            showToast("Configuration reloaded successfully")
            // Refresh spinner after reload
            isSpinnerInitialized = false
            setupConfigSpinner()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to reload configurations", e)
            showToast("Configuration reload failed: ${e.message}")
        }
    }
    
    private fun hasAudioPermission(): Boolean {
        return ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
    }
    
    private fun requestAudioPermission() {
        if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.RECORD_AUDIO)) {
            // Show explanation dialog
            AlertDialog.Builder(this)
                .setTitle("Permission Required")
                .setMessage("This app needs microphone access permission to record audio.")
                .setPositiveButton("Grant") { _, _ ->
                    ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), PERMISSION_REQUEST_CODE)
                }
                .setNegativeButton("Cancel", null)
                .show()
        } else {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), PERMISSION_REQUEST_CODE)
        }
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.isNotEmpty()) {
            val message = if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                "Permission granted"
            } else {
                "Recording permission required to use this app"
            }
            showToast(message)
        }
    }
    
    @SuppressLint("SetTextI18n")
    private fun startRecording() {
        if (audioRecorder.isRecording()) {
            showToast("Already recording")
            return
        }
        
        statusText.text = "Preparing to record..."
        audioRecorder.startRecording()
    }
    
    @SuppressLint("SetTextI18n")
    private fun stopRecording() {
        if (!audioRecorder.isRecording()) {
            showToast("Not currently recording")
            return
        }
        
        statusText.text = "Stopping..."
        audioRecorder.stopRecording()
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
            recordingInfoText.text = "Recording Information"
        }
    }
    
    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }
    
    override fun onDestroy() {
        super.onDestroy()
        try {
            audioRecorder.release()
            Log.d(TAG, "AAudioRecorder resources released successfully")
        } catch (e: Exception) {
            Log.e(TAG, "Error releasing AAudioRecorder resources", e)
        }
    }
    
    override fun onPause() {
        super.onPause()
        // Stop recording when app goes to background
        if (audioRecorder.isRecording()) {
            audioRecorder.stopRecording()
            Log.d(TAG, "Recording stopped due to app going to background")
        }
    }
}
