# AAudio Recorder

[中文](README.md) | English

A high-performance audio recording test application based on Android AAudio API, supporting 8
recording configurations and real-time WAV file writing.

## 📋 Overview

AAudioRecorder is an audio recording test tool designed for the Android platform, using Google's
AAudio low-latency audio API. This project demonstrates how to implement high-quality audio
recording in Android applications, supporting various recording scenarios and performance modes.

## ✨ Key Features

- **🎙️ High-Performance Recording**: Low-latency recording based on AAudio API (~10-40ms)
- **🔧 8 Recording Presets**: Covering general, voice, camcorder, high-performance recording
  scenarios
- **📱 Modern UI**: Intuitive control interface with Material Design style
- **🎵 Multi-Format Support**: PCM 16-bit, 24-bit, and float formats
- **⚡ Real-time Processing**: Audio data written to WAV file in real-time, supports continuous
  recording
- **🛠️ Dynamic Configuration**: Runtime switching of recording configurations, JSON config file
  support
- **📝 Smart Naming**: Auto-generated recording filenames with timestamps
- **🏗️ Optimized Architecture**: Clear code structure and modular design

## 🏗️ Technical Architecture

### Core Components

- **MainActivity**: Modern main interface controller with permission management and user interaction
- **AAudioRecorder**: Kotlin-written audio recorder wrapper class with permission management
- **AAudioConfig**: Recording configuration management class with dynamic config loading
- **WavFile**: C++ implemented WAV file writer class supporting real-time writing
- **Native Engine**: C++ implemented AAudio recording engine

### Technology Stack

- **Language**: Kotlin + C++
- **Audio API**: Android AAudio
- **Build System**: Gradle + CMake
- **Minimum Version**: Android 12L (API 32)
- **Target Version**: Android 15 (API 36)
- **NDK Version**: 29.0.14206865
- **Java Version**: Java 21

## 🎙️ Supported Recording Scenarios

### 8 Preset Configurations

1. **Generic Recording** - Standard recording scenario (48kHz mono, low latency)
2. **Camcorder Recording** - Video recording audio (48kHz stereo, power saving)
3. **Voice Recognition** - Voice recognition optimized (16kHz mono, low latency)
4. **Voice Communication** - Voice communication optimized (16kHz mono, low latency)
5. **Unprocessed Recording** - Raw recording without processing (48kHz stereo, 16-bit, exclusive
   mode)
6. **Voice Performance** - Professional voice recording (48kHz mono, exclusive mode)
7. **Echo Reference** - Echo reference for AEC (48kHz stereo, exclusive mode)
8. **Hotword Detection** - Low-power hotword detection (16kHz mono, power saving)

## 🚀 Quick Start

### System Requirements

- Android 12L (API 32) or higher
- Device with AAudio support
- Development Environment: Android Studio

### Permission Requirements

- `RECORD_AUDIO`: Recording permission (required for core functionality)
- `READ_EXTERNAL_STORAGE`: Read external storage permission (Android 12 and below, for reading
  config files)
- `WRITE_EXTERNAL_STORAGE`: Write external storage permission (Android 9 and below, for saving
  recording files)

### Installation Steps

1. **Clone Project**
   ```bash
   git clone https://github.com/kainan-tek/AAudioRecorder.git
   cd AAudioRecorder
   ```

2. **Build and Install**
   ```bash
   ./gradlew assembleDebug
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

3. **Run the App**
    - The app will automatically request recording permissions on first run
    - Follow the on-screen prompts to grant the required permissions

## 📖 Usage Guide

### Basic Operations

1. **Recording Control**
    - 🎙️ **Start Recording**: Tap the green recording button
    - ⏹️ **Stop Recording**: Tap the red stop button
    - ⚙️ **Recording Config**: Tap config button to switch recording settings

2. **Configuration Management**
    - Auto-load configurations on app startup
    - Support dynamic loading from external files
    - Switch between different recording scenarios via dropdown menu at runtime
    - Long-press config dropdown to reload external config file

### UI Features

- **Status Display**: Real-time display of recording status and audio parameters
- **Config Selection**: Select different recording configurations via dropdown menu
- **Permission Management**: Auto-check and request necessary permissions
- **Config Reload**: Long-press dropdown menu to reload external config file

## 🔧 Configuration File

### Configuration Location

- **External Config**: `/data/aaudio_recorder_configs.json` (priority)
- **Built-in Config**: `app/src/main/assets/aaudio_recorder_configs.json`

### Configuration Format

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
      "outputPath": "/data/recorded_48k_1ch_16bit.wav",
      "description": "Generic Recording - 48kHz Mono 16-bit"
    }
  ]
}
```

### Supported Constant Values

**Input Preset:**

- `AAUDIO_INPUT_PRESET_GENERIC` - Generic recording
- `AAUDIO_INPUT_PRESET_CAMCORDER` - Camcorder recording
- `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION` - Voice recognition
- `AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION` - Voice communication
- `AAUDIO_INPUT_PRESET_UNPROCESSED` - Unprocessed recording
- `AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE` - Voice performance
- `AAUDIO_INPUT_PRESET_SYSTEM_ECHO_REFERENCE` - Echo reference
- `AAUDIO_INPUT_PRESET_SYSTEM_HOTWORD` - Hotword detection

**Format:**

- `16` - 16-bit integer (AAUDIO_FORMAT_PCM_I16)
- `24` - 24-bit integer (AAUDIO_FORMAT_PCM_I24_PACKED)
- `32` - 32-bit integer (AAUDIO_FORMAT_PCM_I32)
- `FLOAT` - 32-bit float (AAUDIO_FORMAT_PCM_FLOAT)

**Performance Mode:**

- `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` - Low latency mode
- `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` - Power saving mode

**Sharing Mode:**

- `AAUDIO_SHARING_MODE_EXCLUSIVE` - Exclusive mode
- `AAUDIO_SHARING_MODE_SHARED` - Shared mode

## 📝 Smart File Naming

### Auto-Naming Rules

When `outputPath` in configuration is empty, the system auto-generates a timestamped filename at
recording start:

```
rec_YYYYMMDD_HHMMSS_mmm_[sampleRate]k_[channels]ch_[bitDepth]bit.wav
```

**Example Filenames:**

- `rec_20240124_143052_123_48k_1ch_16bit.wav`
- `rec_20240124_143052_456_16k_1ch_16bit.wav`
- `rec_20240124_143052_789_48k_2ch_24bit.wav`

### File Path Rules

- **Specified Path**: Use the complete `outputPath` from configuration
- **Auto Path**: Save to app's default storage directory (`getExternalFilesDir(null)`)
- **Permission Requirement**: Ensure app has write permission

## 🔍 Technical Details

### AAudio Integration

- Callback mode for low-latency recording
- Multiple audio format support (16/24-bit PCM and float)
- Complete error handling mechanism
- Real-time WAV file writing

### Data Flow Architecture

```
Microphone → AAudio Stream → Audio Callback → WavFileWriter → WAV File
                                  ↓
                             JNI Callback → Kotlin UI Update
```

### WAV File Writing

- **Real-time Writing**: Continuous audio data writing during recording
- **Format Support**: Standard RIFF/WAVE format
- **Multi-channel Support**: 1-16 channel recording
- **Sample Rate Range**: 8kHz - 192kHz
- **Bit Depth Support**: 8/16/24/32-bit and float

## 📚 API Reference

### AAudioRecorder Class

```kotlin
class AAudioRecorder {
    fun setAudioConfig(config: AAudioConfig)            // Set configuration
    fun startRecording(): Boolean                       // Start recording
    fun stopRecording(): Boolean                        // Stop recording
    fun isRecording(): Boolean                          // Check recording status
    fun setRecordingListener(listener: RecordingListener?) // Set listener
}
```

### AAudioConfig Class

```kotlin
data class AAudioConfig(
    val inputPreset: String,                    // Input preset
    val sampleRate: Int,                        // Sample rate
    val channelCount: Int,                      // Channel count
    val format: Any,                            // Audio format
    val performanceMode: String,                // Performance mode
    val sharingMode: String,                    // Sharing mode
    val outputPath: String,                     // Output path
    val description: String                     // Config description
)
```

## 🐛 Troubleshooting

### Common Issues

1. **Recording Failure**
    - Confirm recording permission granted
    - Check device microphone functionality
    - Verify configuration parameters

2. **Permission Issues**
    - The app will automatically request permissions on first run, follow the on-screen prompts
    - If permissions are denied, manually grant recording permission in system settings
    - Use `adb shell pm grant com.example.aaudiorecorder android.permission.RECORD_AUDIO` to
      manually grant permission
    - Use `adb shell setenforce 0` to temporarily disable SELinux
    - Check storage permissions

3. **Config Loading Failure**
    - Check JSON format correctness
    - Verify config file path
    - View log output

4. **File Write Failure**
    - Ensure output directory exists
    - Check write permissions
    - Verify disk space

### Debug Information

```bash
adb logcat -s AAudioRecorder MainActivity
```

### Log Tags

- `AAudioRecorder`: Recorder related logs
- `MainActivity`: Main interface related logs
- `AAudioConfig`: Configuration related logs

## 📊 Performance Metrics

- **Low Latency Mode**: ~10-40ms
- **Power Saving Mode**: ~80-120ms
- **Sample Rate**: 8kHz - 192kHz
- **Channel Count**: 1-16 channels
- **Bit Depth**: 8/16/24/32-bit and float
- **Supported Format**: PCM WAV file

## 🔗 Related Projects

- [**AAudioPlayer**](https://github.com/kainan-tek/AAudioPlayer) - Companion AAudio player project
- [**AudioPlayer**](https://github.com/kainan-tek/AudioPlayer) - Basic audio player project
- [**AudioRecorder**](https://github.com/kainan-tek/AudioRecorder) - Basic audio recorder project

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Note**: This project is for learning and testing purposes only. Please ensure use in appropriate
devices and environments, and comply with relevant recording laws and regulations.
