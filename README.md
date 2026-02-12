# AAudio Recorder

中文 | [English](README_EN.md)

一个基于Android AAudio API的高性能音频录制器测试程序，支持8种录音配置和实时WAV文件写入。

## 📋 项目概述

AAudio Recorder是一个专为Android平台设计的音频录制测试工具，使用Google的AAudio低延迟音频API。该项目展示了如何在Android应用中实现高质量的音频录制，支持多种录音使用场景和性能模式。

## ✨ 主要特性

- **🎙️ 高性能录音**: 基于AAudio API实现低延迟录制 (~10-40ms)
- **🔧 8种录音预设**: 涵盖通用、语音、摄像、高性能等录音场景
- **📱 现代化界面**: Material Design风格的直观控制界面
- **🎵 多格式支持**: 支持PCM 16位、24位和浮点格式
- **⚡ 实时处理**: 音频数据实时写入WAV文件，支持连续录制
- **🛠️ 动态配置**: 运行时切换录音配置，支持JSON配置文件
- **📝 智能命名**: 自动生成带时间戳的录音文件名
- **🏗️ 优化架构**: 清晰的代码结构和模块化设计

## 🏗️ 技术架构

### 核心组件

- **AAudioRecorder**: Kotlin编写的音频录制器封装类，集成权限管理
- **AAudioConfig**: 录音配置管理类，支持动态加载配置
- **MainActivity**: 现代化主界面控制器，提供权限管理和用户交互
- **WavFileWriter**: C++实现的WAV文件写入器，支持实时写入
- **Native Engine**: C++实现的AAudio录音引擎

### 技术栈

- **语言**: Kotlin + C++
- **音频API**: Android AAudio
- **构建系统**: Gradle + CMake
- **最低版本**: Android 12L (API 32)
- **目标版本**: Android 15 (API 36)
- **NDK版本**: 29.0.14206865
- **Java版本**: Java 21

## 🎙️ 支持的录音场景

### 8种预设配置

1. **通用录音** - 标准录音场景 (48kHz单声道，低延迟)
2. **摄像录音** - 视频录制音频 (48kHz立体声，省电模式)
3. **语音识别** - 语音识别优化 (16kHz单声道，低延迟)
4. **语音通话** - 语音通信优化 (16kHz单声道，低延迟)
5. **原始录音** - 无处理录音 (48kHz立体声，24位，独占模式)
6. **高性能语音** - 专业语音录制 (48kHz单声道，独占模式)
7. **回声参考** - 用于AEC的回声参考 (48kHz立体声，独占模式)
8. **热词检测** - 低功耗热词检测 (16kHz单声道，省电模式)

## 🚀 快速开始

### 系统要求

- Android 12L (API 32) 或更高版本
- 支持AAudio的设备
- 开发环境: Android Studio

### 权限要求

- `RECORD_AUDIO`: 录音权限 (核心功能必需)
- `READ_MEDIA_AUDIO` (API 33+): 读取音频文件
- `READ_EXTERNAL_STORAGE` (API ≤32): 读取外部存储

### 安装步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/kainan-tek/AAudioRecorder.git
   cd AAudioRecorder
   ```

2. **编译安装**
   ```bash
   ./gradlew assembleDebug
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

3. **授予权限**
   ```bash
   adb shell pm grant com.example.aaudiorecorder android.permission.RECORD_AUDIO
   ```

## 📖 使用说明

### 基本操作

1. **录音控制**
   - 🎙️ **开始录音**: 点击绿色录音按钮
   - ⏹️ **停止录音**: 点击红色停止按钮
   - ⚙️ **录音配置**: 点击配置按钮切换录音设置

2. **配置管理**
   - 应用启动时自动加载配置
   - 支持从外部文件动态加载配置
   - 可在运行时通过下拉菜单切换不同的录音场景
   - 长按配置下拉菜单可重新加载外部配置文件

### 界面功能

- **状态显示**: 实时显示录音状态和音频参数
- **配置选择**: 通过下拉菜单选择不同的录音配置
- **权限管理**: 自动检查和申请必要权限
- **配置重载**: 长按下拉菜单重新加载外部配置文件

## 🔧 配置文件

### 配置位置

- **外部配置**: `/data/aaudio_recorder_configs.json` (优先)
- **内置配置**: `app/src/main/assets/aaudio_recorder_configs.json`

### 配置格式

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
      "description": "标准录音配置"
    }
  ]
}
```

### 支持的常量值

**Input Preset (输入预设):**
- `AAUDIO_INPUT_PRESET_GENERIC` - 通用录音
- `AAUDIO_INPUT_PRESET_CAMCORDER` - 摄像录音
- `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION` - 语音识别
- `AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION` - 语音通话
- `AAUDIO_INPUT_PRESET_UNPROCESSED` - 原始录音
- `AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE` - 高性能语音
- `AAUDIO_INPUT_PRESET_SYSTEM_ECHO_REFERENCE` - 回声参考
- `AAUDIO_INPUT_PRESET_SYSTEM_HOTWORD` - 热词检测

**Format (音频格式):**
- `16` - 16位整数 (AAUDIO_FORMAT_PCM_I16)
- `24` - 24位整数 (AAUDIO_FORMAT_PCM_I24_PACKED)
- `32` - 32位整数 (AAUDIO_FORMAT_PCM_I32)
- `FLOAT` - 32位浮点 (AAUDIO_FORMAT_PCM_FLOAT)

**Performance Mode (性能模式):**
- `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` - 低延迟模式
- `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` - 省电模式

**Sharing Mode (共享模式):**
- `AAUDIO_SHARING_MODE_EXCLUSIVE` - 独占模式
- `AAUDIO_SHARING_MODE_SHARED` - 共享模式

## 📝 智能文件命名

### 自动命名规则

当配置中的 `outputPath` 为空时，系统会自动生成文件名：

```
rec_YYYYMMDD_HHMMSS_mmm_[sampleRate]k_[channels]_[bitDepth]bit.wav
```

**示例文件名:**
- `rec_20240124_143052_123_48k_mono_16bit.wav`
- `rec_20240124_143052_456_16k_mono_16bit.wav`
- `rec_20240124_143052_789_48k_stereo_24bit.wav`

### 文件路径规则

- **指定路径**: 使用配置中的 `outputPath`
- **自动路径**: 保存到 `/data/` 目录下
- **权限要求**: 确保应用有写入权限

## 🔍 技术细节

### AAudio集成

- 使用callback模式实现低延迟录制
- 支持多种音频格式 (16/24位PCM和浮点)
- 完整的错误处理机制
- 实时WAV文件写入

### 数据流架构

```
麦克风 → AAudio Stream → Audio Callback → WavFileWriter → WAV文件
                              ↓
                         JNI回调 → Kotlin UI更新
```

### WAV文件写入

- **实时写入**: 录音过程中持续写入音频数据
- **格式支持**: 标准RIFF/WAVE格式
- **多声道支持**: 1-16声道录制
- **采样率范围**: 8kHz - 192kHz
- **位深度支持**: 8/16/24/32位和浮点

## 📚 API 参考

### AAudioRecorder 类
```kotlin
class AAudioRecorder {
    fun setConfig(config: AAudioConfig)                 // 设置配置
    fun startRecording(): Boolean                       // 开始录音
    fun stopRecording(): Boolean                        // 停止录音
    fun isRecording(): Boolean                          // 检查录音状态
    fun setRecordingListener(listener: RecordingListener?) // 设置监听器
}
```

### AAudioConfig 类
```kotlin
data class AAudioConfig(
    val inputPreset: String,                    // 输入预设
    val sampleRate: Int,                        // 采样率
    val channelCount: Int,                      // 声道数
    val format: Any,                            // 音频格式
    val performanceMode: String,                // 性能模式
    val sharingMode: String,                    // 共享模式
    val outputPath: String,                     // 输出路径
    val description: String                     // 配置描述
)
```

## 🐛 故障排除

### 常见问题

1. **录音失败**
   - 确认已授予录音权限
   - 检查设备麦克风是否正常
   - 验证配置参数是否正确

2. **权限问题**
   - 手动授予录音权限
   - 使用 `adb shell setenforce 0` 临时禁用SELinux
   - 检查存储权限

3. **配置加载失败**
   - 检查JSON格式是否正确
   - 验证配置文件路径
   - 查看日志输出

4. **文件写入失败**
   - 确保输出目录存在
   - 检查写入权限
   - 验证磁盘空间

### 调试信息
```bash
adb logcat -s AAudioRecorder MainActivity
```

### 日志标签
- `AAudioRecorder`: 录制器相关日志
- `MainActivity`: 主界面相关日志
- `AAudioConfig`: 配置相关日志

## 📊 性能指标

- **低延迟模式**: ~10-40ms
- **省电模式**: ~80-120ms
- **采样率**: 8kHz - 192kHz
- **声道数**: 1-16声道
- **位深度**: 8/16/24/32位和浮点
- **支持格式**: PCM WAV文件

## 🔗 相关项目

- [AAudioPlayer](../AAudioPlayer/) - 配套的AAudio播放器项目
- [AudioPlayer](../AudioPlayer/) - 基础音频播放器项目
- [AudioRecorder](../AudioRecorder/) - 基础音频录制器项目

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

---

**注意**: 本项目仅用于学习和测试目的，请确保在合适的设备和环境中使用，并遵守相关的录音法律法规。