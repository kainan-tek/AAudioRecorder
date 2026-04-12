# AAudioRecorder

[中文文档](README.md) | English

A high-performance audio recorder based on Android AAudio API, supporting 8 recording scenario
configurations.

## Table of Contents

- [Introduction](#introduction)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Configuration](#configuration)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [License](#license)
- [Contact](#contact)

## Introduction

AAudioRecorder is an Android high-performance audio recorder based on Android AAudio Native API,
designed for low-latency audio recording scenarios.

### Key Features

- **8 Recording Scenarios**: Generic, camcorder, voice recognition, voice communication, etc.
- **Complete Audio Support**: 1-16 channels, 8kHz-192kHz sample rates, 16/24/32-bit PCM
- **WAV File Output**: Automatic WAV header generation with smart file naming
- **Low Latency Mode**: LOW_LATENCY performance mode with latency as low as 10-40ms
- **Flexible Configuration**: JSON configuration file with external hot-reload support
- **Native Implementation**: C++ implementation with JNI callbacks, high performance and low
  overhead

### Recording Scenarios

| Scenario              | Input Preset        | Sample Rate | Channels | Typical Use            |
|-----------------------|---------------------|-------------|----------|------------------------|
| Generic Recording     | GENERIC             | 48kHz       | Mono     | Standard recording     |
| Camcorder Recording   | CAMCORDER           | 48kHz       | Stereo   | Video recording        |
| Voice Recognition     | VOICE_RECOGNITION   | 16kHz       | Mono     | ASR applications       |
| Voice Communication   | VOICE_COMMUNICATION | 16kHz       | Mono     | VoIP applications      |
| Unprocessed Recording | UNPROCESSED         | 48kHz       | Stereo   | Raw signal recording   |
| Voice Performance     | VOICE_PERFORMANCE   | 48kHz       | Mono     | Professional recording |
| Echo Reference        | ECHO_REFERENCE      | 48kHz       | Stereo   | AEC reference          |
| Hotword Detection     | HOTWORD             | 16kHz       | Mono     | Low-power detection    |

## Quick Start

### Basic Usage

1. **Select Config** - Choose recording scenario via dropdown menu
2. **Start Recording** - Tap green record button
3. **Stop Recording** - Tap red stop button
4. **Reload Config** - Long-press dropdown to reload external config

### Common Operations

```bash
# View recording logs
adb logcat -s AAudioRecorder MainActivity AAudioConfig

# Check config file
adb shell cat /data/aaudio_recorder_configs.json

# View recording files
adb shell ls -la /data/recorded_*.wav
```

## Installation

### Requirements

- **Android Version**: Android 12L (API 32) or higher
- **Development Environment**: Android Studio + NDK 29.0+
- **Build System**: Gradle + CMake

### Build and Install

```bash
git clone https://github.com/kainan-tek/AAudioRecorder.git
cd AAudioRecorder
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

### Permissions

| Permission               | Purpose           | Version     |
|--------------------------|-------------------|-------------|
| `RECORD_AUDIO`           | Recording         | All         |
| `READ_EXTERNAL_STORAGE`  | Read config files | Android 12- |
| `WRITE_EXTERNAL_STORAGE` | Save recordings   | Android 9-  |

```bash
# Grant recording permission manually
adb shell pm grant com.example.aaudiorecorder android.permission.RECORD_AUDIO
```

## Configuration

### Config File Location

- **External Config**: `/data/aaudio_recorder_configs.json` (loaded first)
- **Built-in Config**: `app/src/main/assets/aaudio_recorder_configs.json`

### Config File Format

```json
{
  "configs": [
    {
      "inputPreset": "AAUDIO_INPUT_PRESET_GENERIC",
      "sampleRate": 48000,
      "channelCount": 1,
      "format": 16,
      "performanceMode": "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
      "sharingMode": "AAUDIO_SHARING_MODE_SHARED",
      "audioFilePath": "/data/recorded_48k_1ch_16bit.wav",
      "description": "Generic Recording"
    }
  ]
}
```

### Configuration Parameters

#### Input Preset

| Value                                       | Description           | Typical Use            |
|---------------------------------------------|-----------------------|------------------------|
| `AAUDIO_INPUT_PRESET_GENERIC`               | Generic recording     | Standard recording     |
| `AAUDIO_INPUT_PRESET_CAMCORDER`             | Camcorder recording   | Video recording        |
| `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION`     | Voice recognition     | ASR applications       |
| `AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION`   | Voice communication   | VoIP applications      |
| `AAUDIO_INPUT_PRESET_UNPROCESSED`           | Unprocessed recording | Raw signal recording   |
| `AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE`     | Voice performance     | Professional recording |
| `AAUDIO_INPUT_PRESET_SYSTEM_ECHO_REFERENCE` | Echo reference        | AEC reference          |
| `AAUDIO_INPUT_PRESET_SYSTEM_HOTWORD`        | Hotword detection     | Low-power detection    |

#### Format

| Value | Description        |
|-------|--------------------|
| `16`  | 16-bit integer PCM |
| `24`  | 24-bit integer PCM |
| `32`  | 32-bit integer PCM |

> **Note**: The `format` field in config file uses bit depth integer values (16/24/32), string
> format is not supported.

#### Performance Mode

| Value                                  | Description       | Typical Latency |
|----------------------------------------|-------------------|-----------------|
| `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`  | Low latency mode  | ~10-40ms        |
| `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` | Power saving mode | ~80-120ms       |
| `AAUDIO_PERFORMANCE_MODE_NONE`         | Default mode      | System default  |

### Smart File Naming

When `audioFilePath` is empty, auto-generated filename format:

```
rec_YYYYMMDD_HHMMSS_mmm_[sampleRate]k_[channels]ch_[bitDepth]bit.wav
```

**Example**: `rec_20240124_143052_123_48k_1ch_16bit.wav`

## API Reference

### AAudioRecorder Class

```kotlin
class AAudioRecorder(private val context: Context) {
    fun setAudioConfig(config: AAudioConfig)   // Set audio configuration
    fun startRecording(): Boolean              // Start recording
    fun stopRecording()                        // Stop recording (idempotent)
    fun isRecording(): Boolean                 // Check recording status
    fun release()                              // Release resources
    fun setRecordingListener(listener: RecordingListener?)  // Set listener
}
```

### RecordingListener Interface

```kotlin
interface RecordingListener {
    fun onRecordingStarted()                   // Recording started callback
    fun onRecordingStopped()                   // Recording stopped callback
    fun onRecordingError(error: String)        // Recording error callback
}
```

### Error Prefixes

| Prefix    | Description                |
|-----------|----------------------------|
| `[PARAM]` | Parameter validation error |

> **Note**: `[FILE]`, `[STREAM]`, `[PERMISSION]` errors are used in Native layer only, not exposed
> to Java/Kotlin layer.

## Troubleshooting

### Common Issues

#### 1. Recording Failed

```bash
# Check recording permission
adb shell dumpsys package com.example.aaudiorecorder | grep RECORD_AUDIO

# View detailed logs
adb logcat -s AAudioRecorder aaudio_recorder
```

#### 2. Permission Issues

```bash
adb shell pm grant com.example.aaudiorecorder android.permission.RECORD_AUDIO
adb shell setenforce 0
```

#### 3. File Save Failed

```bash
adb shell df /data          # Check disk space
adb shell ls -la /data/     # Check file permissions
```

### Debug Commands

```bash
adb logcat -s AAudioRecorder MainActivity AAudioConfig aaudio_recorder
adb logcat -s AAudio
```

## Related Projects

- [AAudioPlayer](https://github.com/kainan-tek/AAudioPlayer) - High-performance player based on
  AAudio API
- [AudioPlayer](https://github.com/kainan-tek/AudioPlayer) - Audio player based on AudioTrack API
- [AudioRecorder](https://github.com/kainan-tek/AudioRecorder) - Audio recorder based on AudioRecord
  API
- [audio_test_client](https://github.com/kainan-tek/audio_test_client) - Android system-level audio
  testing tool

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

**Note**: This project is for learning and testing purposes only. Please comply with relevant
recording laws and regulations. AAudio API requires Android 12L (API 32) or higher.

## Contact 

 - **Author**: kainan-tek 
 - **Email**: kainanos@outlook.com 
 - **GitHub**: https://github.com/kainan-tek/AAudioRecorder 
 - **Issue**: `https://github.com/kainan-tek/AAudioRecorder/issues` 

 ---

 <div align="center"> 

 **If this project helps you, please give it a ⭐ Star!** 

 Made with ❤️ by kainan-tek 

 [⬆ Back to top](#aaudiorecorder) 

 </div>
