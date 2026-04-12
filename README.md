# AAudioRecorder

中文 | [English](README_EN.md)

基于 Android AAudio API 的高性能音频录制器，支持 8 种录音场景配置。

## 目录

- [项目简介](#项目简介)
- [快速开始](#快速开始)
- [安装部署](#安装部署)
- [配置说明](#配置说明)
- [API 参考](#api-参考)
- [故障排除](#故障排除)
- [许可证](#许可证)
- [联系方式](#联系方式)

## 项目简介

AAudioRecorder 是一个 Android 高性能音频录制器，基于 Android AAudio Native API 开发，适用于低延迟音频录制场景。

### 核心特性

- **8 种录音场景**: 通用录音、摄像录音、语音识别、语音通话等
- **完整音频支持**: 1-16 声道，8kHz-192kHz 采样率，16/24/32 位 PCM
- **WAV 文件输出**: 自动生成 WAV 文件头，支持智能文件命名
- **低延迟模式**: 支持 LOW_LATENCY 性能模式，延迟可低至 10-40ms
- **灵活配置**: JSON 配置文件，支持外部热更新
- **Native 实现**: C++ 实现，JNI 回调，高性能低开销

### 录音场景

| 场景    | Input Preset        | 采样率   | 声道  | 典型用途    |
|-------|---------------------|-------|-----|---------|
| 通用录音  | GENERIC             | 48kHz | 单声道 | 标准录音    |
| 摄像录音  | CAMCORDER           | 48kHz | 立体声 | 视频录制    |
| 语音识别  | VOICE_RECOGNITION   | 16kHz | 单声道 | ASR 应用  |
| 语音通话  | VOICE_COMMUNICATION | 16kHz | 单声道 | VoIP 应用 |
| 原始录音  | UNPROCESSED         | 48kHz | 立体声 | 无处理录音   |
| 高性能语音 | VOICE_PERFORMANCE   | 48kHz | 单声道 | 专业录制    |
| 回声参考  | ECHO_REFERENCE      | 48kHz | 立体声 | AEC 参考  |
| 热词检测  | HOTWORD             | 16kHz | 单声道 | 低功耗检测   |

## 快速开始

### 基本使用

1. **选择配置** - 通过下拉菜单选择录音场景
2. **开始录音** - 点击绿色录音按钮
3. **停止录音** - 点击红色停止按钮
4. **重载配置** - 长按下拉菜单重新加载外部配置

### 常用操作

```bash
# 查看录音日志
adb logcat -s AAudioRecorder MainActivity AAudioConfig

# 检查配置文件
adb shell cat /data/aaudio_recorder_configs.json

# 查看录音文件
adb shell ls -la /data/recorded_*.wav
```

## 安装部署

### 环境要求

- **Android 版本**: Android 12L (API 32) 或更高
- **开发环境**: Android Studio + NDK 29.0+
- **构建系统**: Gradle + CMake

### 编译安装

```bash
git clone https://github.com/kainan-tek/AAudioRecorder.git
cd AAudioRecorder
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

### 权限配置

| 权限                       | 用途     | 版本要求        |
|--------------------------|--------|-------------|
| `RECORD_AUDIO`           | 录音权限   | 全部          |
| `READ_EXTERNAL_STORAGE`  | 读取配置文件 | Android 12- |
| `WRITE_EXTERNAL_STORAGE` | 保存录音文件 | Android 9-  |

```bash
# 手动授予录音权限
adb shell pm grant com.example.aaudiorecorder android.permission.RECORD_AUDIO
```

## 配置说明

### 配置文件位置

- **外部配置**: `/data/aaudio_recorder_configs.json`（优先加载）
- **内置配置**: `app/src/main/assets/aaudio_recorder_configs.json`

### 配置文件格式

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

### 配置参数

#### Input Preset（输入预设）

| 值                                           | 说明    | 典型用途    |
|---------------------------------------------|-------|---------|
| `AAUDIO_INPUT_PRESET_GENERIC`               | 通用录音  | 标准录音场景  |
| `AAUDIO_INPUT_PRESET_CAMCORDER`             | 摄像录音  | 视频录制    |
| `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION`     | 语音识别  | ASR 应用  |
| `AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION`   | 语音通话  | VoIP 应用 |
| `AAUDIO_INPUT_PRESET_UNPROCESSED`           | 原始录音  | 无处理录音   |
| `AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE`     | 高性能语音 | 专业录制    |
| `AAUDIO_INPUT_PRESET_SYSTEM_ECHO_REFERENCE` | 回声参考  | AEC 参考  |
| `AAUDIO_INPUT_PRESET_SYSTEM_HOTWORD`        | 热词检测  | 低功耗检测   |

#### Format（音频格式）

| 值    | 说明         |
|------|------------|
| `16` | 16 位整数 PCM |
| `24` | 24 位整数 PCM |
| `32` | 32 位整数 PCM |

> **注意**: 配置文件中 `format` 字段使用位深度整数值（16/24/32），不支持字符串格式。

#### Performance Mode（性能模式）

| 值                                      | 说明    | 典型延迟      |
|----------------------------------------|-------|-----------|
| `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`  | 低延迟模式 | ~10-40ms  |
| `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` | 省电模式  | ~80-120ms |
| `AAUDIO_PERFORMANCE_MODE_NONE`         | 默认模式  | 系统默认      |

### 智能文件命名

当 `audioFilePath` 为空时，自动生成带时间戳的文件名：

```
rec_YYYYMMDD_HHMMSS_mmm_[sampleRate]k_[channels]ch_[bitDepth]bit.wav
```

**示例**: `rec_20240124_143052_123_48k_1ch_16bit.wav`

## API 参考

### AAudioRecorder 类

```kotlin
class AAudioRecorder(private val context: Context) {
    fun setAudioConfig(config: AAudioConfig)   // 设置音频配置
    fun startRecording(): Boolean              // 开始录音
    fun stopRecording()                        // 停止录音（幂等）
    fun isRecording(): Boolean                 // 检查录音状态
    fun release()                              // 释放资源
    fun setRecordingListener(listener: RecordingListener?)  // 设置监听器
}
```

### RecordingListener 接口

```kotlin
interface RecordingListener {
    fun onRecordingStarted()                   // 录音开始回调
    fun onRecordingStopped()                   // 录音停止回调
    fun onRecordingError(error: String)        // 录音错误回调
}
```

### 错误前缀

| 前缀        | 说明     |
|-----------|--------|
| `[PARAM]` | 参数验证错误 |

> **注意**: `[FILE]`、`[STREAM]`、`[PERMISSION]` 错误仅在 Native 层使用，Java/Kotlin 层不直接暴露。

## 故障排除

### 常见问题

#### 1. 录音失败

```bash
# 检查录音权限
adb shell dumpsys package com.example.aaudiorecorder | grep RECORD_AUDIO

# 查看详细日志
adb logcat -s AAudioRecorder aaudio_recorder
```

#### 2. 权限问题

```bash
adb shell pm grant com.example.aaudiorecorder android.permission.RECORD_AUDIO
adb shell setenforce 0
```

#### 3. 文件保存失败

```bash
adb shell df /data          # 检查磁盘空间
adb shell ls -la /data/     # 检查文件权限
```

### 调试命令

```bash
adb logcat -s AAudioRecorder MainActivity AAudioConfig aaudio_recorder
adb logcat -s AAudio
```

## 相关项目

- [AAudioPlayer](https://github.com/kainan-tek/AAudioPlayer) - 基于 AAudio API 的高性能播放器
- [AudioPlayer](https://github.com/kainan-tek/AudioPlayer) - 基于 AudioTrack API 的音频播放器
- [AudioRecorder](https://github.com/kainan-tek/AudioRecorder) - 基于 AudioRecord API 的音频录制器
- [audio_test_client](https://github.com/kainan-tek/audio_test_client) - Android 系统级音频测试工具

## 许可证

本项目采用 MIT License 许可证。详细信息请参阅 [LICENSE](LICENSE) 文件。

**注意**: 本项目仅供学习和测试使用，请遵守相关录音法律法规。AAudio API 需要 Android 12L (API 32)
或更高版本。

## 联系方式 

 - **作者**: kainan-tek 
 - **邮箱**: kainanos@outlook.com 
 - **GitHub**: https://github.com/kainan-tek/AAudioRecorder 
 - **问题反馈**: `https://github.com/kainan-tek/AAudioRecorder/issues` 

 ---

 <div align="center"> 

 **如果这个项目对你有帮助，请给个 ⭐ Star！** 

 Made with ❤️ by kainan-tek 

 [⬆ 回到顶部](#aaudiorecorder) 

 </div>
