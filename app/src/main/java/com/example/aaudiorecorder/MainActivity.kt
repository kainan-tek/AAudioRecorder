package com.example.aaudiorecorder

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.os.Build
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
import androidx.core.content.ContextCompat
import com.example.aaudiorecorder.config.AAudioConfig
import com.example.aaudiorecorder.recorder.AAudioRecorder

/**
 * AAudio Recorder Main Activity
 *
 * Usage Instructions:
 * 1. Ensure device supports AAudio API
 * 2. Grant recording permissions
 * 3. Select recording configuration from dropdown
 * 4. Start recording
 *
 * System Requirements: Android 12L (API 32+)
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
        audioRecorder = AAudioRecorder(this)
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
                    handleError(error)
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
     * Reload configuration file
     */
    @SuppressLint("SetTextI18n")
    private fun reloadConfigurations() {
        try {
            val previousDescription = currentConfig?.description
            availableConfigs = AAudioConfig.reloadConfigs(this)
            if (availableConfigs.isNotEmpty()) {
                // Try to keep previous selection if it still exists
                currentConfig = previousDescription?.let { desc ->
                    availableConfigs.find { it.description == desc }
                } ?: availableConfigs[0]
                audioRecorder.setAudioConfig(currentConfig!!)
                isSpinnerInitialized = false
                setupConfigSpinner()
                updateRecordingInfo()
                showToast("Configuration reloaded successfully")
                statusText.text = "Ready to record"
                Log.i(TAG, "Reloaded ${availableConfigs.size} recording configurations")
            } else {
                showToast("No valid configurations found")
                statusText.text = "Recording configuration load failed"
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to reload configurations", e)
            showToast("Configuration reload failed: ${e.message}")
        }
    }

    /**
     * Setup configuration spinner
     */
    private fun setupConfigSpinner() {
        val configs = availableConfigs

        if (configs.isEmpty()) {
            Log.w(TAG, "No configurations available")
            return
        }

        val configNames = configs.map { it.description }

        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, configNames)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        configSpinner.adapter = adapter

        // Set initial selection
        currentConfig?.let {
            val index = configs.indexOfFirst { config -> config.description == it.description }
            if (index >= 0) {
                configSpinner.setSelection(index)
            }
        }

        configSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(
                parent: AdapterView<*>?,
                view: View?,
                position: Int,
                id: Long,
            ) {
                if (!isSpinnerInitialized) {
                    isSpinnerInitialized = true
                    return
                }

                val selectedConfig = configs[position]
                currentConfig = selectedConfig
                audioRecorder.setAudioConfig(selectedConfig)
                updateRecordingInfo()
                showToast("Switched to: ${selectedConfig.description}")
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }

        // Add long press listener to reload configurations
        configSpinner.setOnLongClickListener {
            reloadConfigurations()
            true
        }
    }

    /**
     * Get required permissions for recording
     */
    @SuppressLint("ObsoleteSdkInt")
    private fun getRequiredPermissions(): Array<String> {
        return when {
            Build.VERSION.SDK_INT <= Build.VERSION_CODES.P -> {
                arrayOf(
                    Manifest.permission.RECORD_AUDIO,
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
                )
            }

            Build.VERSION.SDK_INT <= Build.VERSION_CODES.S_V2 -> {
                arrayOf(
                    Manifest.permission.RECORD_AUDIO, Manifest.permission.READ_EXTERNAL_STORAGE
                )
            }

            else -> {
                arrayOf(Manifest.permission.RECORD_AUDIO)
            }
        }
    }

    private fun hasAudioPermission(): Boolean {
        return getRequiredPermissions().all {
            ContextCompat.checkSelfPermission(this, it) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestAudioPermission() {
        requestPermissions(getRequiredPermissions(), PERMISSION_REQUEST_CODE)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.isNotEmpty()) {
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            val message = if (allGranted) {
                "Permission granted"
            } else {
                val deniedCount = grantResults.count { it != PackageManager.PERMISSION_GRANTED }
                "Recording permission required ($deniedCount permission(s) denied)"
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
            val filePathDisplay = config.audioFilePath.ifBlank {
                "<App default path (auto-generated at recording start)>"
            }
            val configInfo =
                "Current Config: ${config.description}\n" + "Source: ${config.inputPreset}\n" + "Mode: ${config.performanceMode} | ${config.sharingMode}\n" + "File: $filePathDisplay"
            recordingInfoText.text = configInfo
        } ?: run {
            recordingInfoText.text = "Recording Info"
        }
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }

    /**
     * Handle audio recording errors with user-friendly messages
     */
    @SuppressLint("SetTextI18n")
    private fun handleError(error: String) {
        Log.e(TAG, "Audio recording error: $error")

        val userMessage = getUserFriendlyErrorMessage(error)

        AlertDialog.Builder(this).setTitle("Recording Error").setMessage(userMessage)
            .setPositiveButton("OK") { dialog, _ ->
                dialog.dismiss()
                statusText.text = "Ready to record"
            }.setCancelable(true).setOnCancelListener {
                statusText.text = "Ready to record"
            }.show()

        statusText.text = "Error: $userMessage"
    }

    /**
     * Convert technical error message to user-friendly message
     */
    private fun getUserFriendlyErrorMessage(error: String): String {
        return when {
            error.startsWith(
                "[FILE]", ignoreCase = true
            ) -> "Unable to create recording file. Please check storage permissions and available space."

            error.startsWith(
                "[STREAM]", ignoreCase = true
            ) -> "Audio system initialization failed. Please try again."

            error.startsWith(
                "[PARAM]", ignoreCase = true
            ) -> "Invalid audio configuration. Please select a different configuration."

            error.contains(
                "Already recording", ignoreCase = true
            ) -> "Recording is already in progress."

            error.contains(
                "Not currently recording", ignoreCase = true
            ) -> "No recording is in progress."

            else -> "Recording failed. Please try again."
        }
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
