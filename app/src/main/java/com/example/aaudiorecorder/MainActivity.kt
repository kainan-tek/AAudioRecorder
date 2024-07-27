package com.example.aaudiorecorder

import android.Manifest
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import androidx.core.app.ActivityCompat
// import android.widget.TextView
import com.example.aaudiorecorder.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    // private lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val button1: Button = findViewById(R.id.button1)
        val button2: Button = findViewById(R.id.button2)
        button1.setOnClickListener {
            startAAudioCapture()
        }
        button2.setOnClickListener {
            stopAAudioCapture()
        }

        ActivityCompat.requestPermissions(
            this,
            arrayOf(
                Manifest.permission.RECORD_AUDIO,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
            ),
            0
        )
    }

    companion object {
        private var isStart = false
        private const val LOG_TAG = "AAudioRecorder"
        // Used to load the 'aaudiorecorder' library on application startup.
        init {
            System.loadLibrary("aaudiorecorder")
        }
    }

    private fun startAAudioCapture() {
        if (isStart) {
            Log.i(LOG_TAG, "app in starting status, needn't start again")
            return
        }
        isStart = true

        class AAudioThread : Thread() {
            override fun run() {
                super.run()
                startAAudioCaptureFromJNI()
                isStart = false
            }
        }
        AAudioThread().start()
    }

    private fun stopAAudioCapture() {
        if (!isStart) {
            Log.i(LOG_TAG, "app in stop status, needn't stop again")
            return
        }
        isStart = false
        stopAAudioCaptureFromJNI()
    }

    private external fun startAAudioCaptureFromJNI()
    private external fun stopAAudioCaptureFromJNI()
}
