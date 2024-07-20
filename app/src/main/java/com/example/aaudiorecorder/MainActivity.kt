package com.example.aaudiorecorder

import android.Manifest
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
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

    private fun startAAudioCapture() {
        class AAudioThread : Thread() {
            override fun run() {
                super.run()
                startAAudioCaptureFromJNI()
            }
        }
        AAudioThread().start()
    }

    private fun stopAAudioCapture() {
        stopAAudioCaptureFromJNI()
    }

    private external fun startAAudioCaptureFromJNI()
    private external fun stopAAudioCaptureFromJNI()

    companion object {
        // Used to load the 'aaudiorecorder' library on application startup.
        init {
            System.loadLibrary("aaudiorecorder")
        }
    }
}
