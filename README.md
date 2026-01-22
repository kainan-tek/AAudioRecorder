# AAudio Recorder

一个基于Android AAudio API的高性能音频录制器测试程序，支持多种录音配置和实时音频处理。

## 📋 项目概述

AAudio Recorder是一个专为Android平台设计的音频录制测试工具，使用Google的AAudio低延迟音频API。该项目展示了如何在Android应用中实现高质量的音频录制，支持多种录音使用场景和性能模式。

## ✨ 主要特性

- **🎙️ 高性能录音**: 基于AAudio API实现低延迟音频录制
- **🔧 多种配置**: 支持6种不同的录音使用场景配置
- **📱 简洁界面**: 直观的录音控制界面
- **🎵 多格式支持**: 支持PCM 16位、24位、32位和浮点格式
- **⚡ 实时处理**: 音频数据实时写入WAV文件
- **�️ 动态配置**: 运行时切换录音配置
- **📊 实时状态**: 录音状态实时反馈
- **🛡️ 完善错误处理**: 完整的错误处理和状态管理

## 🏗️ 技术架构

### 核心组件

- **AAudioRecorder**: Kotlin编写的音频录制器封装类
- **RecorderConfig**: 录音配置管理类
- **MainActivity**: 主界面控制器
- **Native录音引擎**: C++编写的AAudio录音实现

### 技术栈

- **语言**: Kotlin + C++14
- **音频API**: Android AAudio
- **构建系统**: Gradle + CMake
- **最低版本**: Android 12L (API 32)
- **目标版本**: Android 15 (API 36)

## 🚀 快速开始

### 系统要求

- Android 12L (API 32) 或更高版本
- 支持AAudio的设备
- 录音权限 (RECORD_AUDIO)
- 开发环境: Android Studio

### 安装步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/kainan-tek/AAudioRecorder.git
   cd AAudioRecorder
   ```

2. **编译安装**
   ```bash
   # 在Android Studio中打开项目
   # 或使用命令行编译
   ./gradlew assembleDebug
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

3. **运行应用**
   - 启动AAudio Recorder应用
   - 授予录音权限
   - 选择录音配置
   - 点击录音按钮

## 📖 使用说明

### 基本操作

1. **录音控制**
   - 🎙️ **录音**: 点击录音按钮开始录制
   - ⏹️ **停止**: 点击停止按钮停止录制
   - ⚙️ **配置**: 点击配置按钮切换录音设置

2. **配置管理**
   - 应用启动时自动加载配置
   - 支持从外部文件动态加载配置
   - 可在运行时切换不同的录音场景

### 录音配置

应用支持以下录音使用场景：

| 配置名称  | Input Preset                            | 采样率   | 声道  | 格式                    | Performance Mode                     | Sharing Mode                  | 说明      |
|-------|-----------------------------------------|-------|-----|-----------------------|--------------------------------------|-------------------------------|---------|
| 标准录音  | AAUDIO_INPUT_PRESET_GENERIC             | 48kHz | 单声道 | AAUDIO_FORMAT_PCM_I16 | AAUDIO_PERFORMANCE_MODE_LOW_LATENCY  | AAUDIO_SHARING_MODE_SHARED    | 通用录音场景  |
| 语音通话  | AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION | 16kHz | 单声道 | AAUDIO_FORMAT_PCM_I16 | AAUDIO_PERFORMANCE_MODE_LOW_LATENCY  | AAUDIO_SHARING_MODE_SHARED    | 语音通话录制  |
| 语音识别  | AAUDIO_INPUT_PRESET_VOICE_RECOGNITION   | 16kHz | 单声道 | AAUDIO_FORMAT_PCM_I16 | AAUDIO_PERFORMANCE_MODE_LOW_LATENCY  | AAUDIO_SHARING_MODE_SHARED    | 语音识别优化  |
| 摄像录音  | AAUDIO_INPUT_PRESET_CAMCORDER           | 48kHz | 立体声 | AAUDIO_FORMAT_PCM_I16 | AAUDIO_PERFORMANCE_MODE_POWER_SAVING | AAUDIO_SHARING_MODE_SHARED    | 视频录制音频  |
| 高性能语音 | AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE   | 48kHz | 单声道 | AAUDIO_FORMAT_PCM_I16 | AAUDIO_PERFORMANCE_MODE_LOW_LATENCY  | AAUDIO_SHARING_MODE_EXCLUSIVE | 专业语音录制  |
| 原始录音  | AAUDIO_INPUT_PRESET_UNPROCESSED         | 48kHz | 立体声 | AAUDIO_FORMAT_PCM_I16 | AAUDIO_PERFORMANCE_MODE_LOW_LATENCY  | AAUDIO_SHARING_MODE_EXCLUSIVE | 无处理原始音频 |

## 🔧 配置文件

### 默认配置位置

- **外部配置**: `/data/aaudio_recorder_configs.json` (优先)
- **内置配置**: `app/src/main/assets/aaudio_recorder_configs.json`

### 配置文件格式

```json
{
  "configs": [
    {
      "inputPreset": "AAUDIO_INPUT_PRESET_GENERIC",
      "sampleRate": 48000,
      "channelCount": 1,
      "format": "AAUDIO_FORMAT_PCM_I16",
      "performanceMode": "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY",
      "sharingMode": "AAUDIO_SHARING_MODE_SHARED",
      "outputPath": "/data/recorded_48k_1ch_16bit.wav",
      "description": "标准录音 - 48kHz单声道"
    }
  ]
}
```

### 配置参数说明

- **inputPreset**: 录音输入预设
  - `AAUDIO_INPUT_PRESET_GENERIC` - 通用录音
  - `AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION` - 语音通话
  - `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION` - 语音识别
  - `AAUDIO_INPUT_PRESET_CAMCORDER` - 摄像录音
  - `AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE` - 高性能语音
  - `AAUDIO_INPUT_PRESET_UNPROCESSED` - 原始录音
- **sampleRate**: 采样率 (Hz)
- **channelCount**: 声道数 (1=单声道, 2=立体声)
- **format**: 音频格式
  - `AAUDIO_FORMAT_PCM_I16` - 16位PCM
  - `AAUDIO_FORMAT_PCM_I24_PACKED` - 24位PCM
  - `AAUDIO_FORMAT_PCM_I32` - 32位PCM
  - `AAUDIO_FORMAT_PCM_FLOAT` - 浮点PCM
- **performanceMode**: 性能模式
  - `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` - 低延迟
  - `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` - 省电模式
- **sharingMode**: 共享模式
  - `AAUDIO_SHARING_MODE_SHARED` - 共享模式
  - `AAUDIO_SHARING_MODE_EXCLUSIVE` - 独占模式
- **outputPath**: 输出文件路径
- **description**: 配置描述

## 🏗️ 项目结构

```
AAudioRecorder/
├── app/
│   ├── src/main/
│   │   ├── cpp/                         # C++源码
│   │   │   ├── aaudio_recorder.cpp      # AAudio录音器实现
│   │   │   ├── aaudio_recorder.h        # 录音器头文件
│   │   │   └── CMakeLists.txt           # CMake构建配置
│   │   ├── java/                        # Kotlin/Java源码
│   │   │   └── com/example/aaudiorecorder/
│   │   │       ├── MainActivity.kt           # 主界面
│   │   │       ├── recorder/
│   │   │       │   └── AAudioRecorder.kt     # 录音器封装
│   │   │       └── config/
│   │   │           └── RecorderConfig.kt     # 配置管理
│   │   ├── res/                         # 资源文件
│   │   └── assets/                      # 资产文件
│   │       └── aaudio_recorder_configs.json # 默认配置
│   └── build.gradle.kts                 # 应用构建配置
├── gradle/                              # Gradle配置
├── build.gradle.kts                    # 项目构建配置
└── README.md                           # 项目文档
```

## 🔍 技术细节

### AAudio集成

- 使用callback模式实现低延迟录音
- 支持多种音频格式 (16/24/32位PCM和浮点)
- 动态缓冲区大小优化
- 完整的错误处理机制
- 实时WAV文件写入

### WAV文件输出

- 标准RIFF/WAVE格式写入
- 支持多声道音频 (1-16声道)
- 采样率范围: 8kHz - 192kHz
- 位深度支持: 8/16/24/32位和浮点
- 实时文件头更新

### 文件路径规则

1. **指定完整路径**: 如果配置中 `outputPath` 以 `.wav` 结尾，直接使用该路径
2. **自动生成**: 如果 `outputPath` 为空，自动生成带时间戳的文件名

### 自动文件名格式
```
/data/rec_YYYYMMDD_HHMMSS_mmm_48k_mono_16bit.wav
```
- `YYYYMMDD_HHMMSS_mmm`: 时间戳（年月日_时分秒_毫秒）
- `48k`: 采样率（kHz）
- `mono/2ch`: 声道数
- `16bit/float`: 音频格式

### 性能优化

- C++14标准实现
- 零拷贝音频数据传输
- 智能缓冲区管理
- 内存使用优化
- 原子操作保证线程安全

## 📚 API 参考

### AAudioRecorder 类
```kotlin
class AAudioRecorder {
    // 设置配置
    fun setConfig(config: RecorderConfig)
    
    // 开始录音
    fun startRecording(): Boolean
    
    // 停止录音
    fun stopRecording(): Boolean
    
    // 检查录音状态
    fun isRecording(): Boolean
    
    // 获取最后录音信息
    fun getLastRecordingInfo(): String
    
    // 设置录音监听器
    fun setRecordingListener(listener: RecordingListener?)
    
    // 释放资源
    fun release()
}
```

### RecorderConfig 类
```kotlin
data class RecorderConfig(
    val inputPreset: String,      // 输入预设
    val sampleRate: Int,          // 采样率
    val channelCount: Int,        // 声道数
    val format: String,           // 音频格式
    val performanceMode: String,  // 性能模式
    val sharingMode: String,      // 共享模式
    val outputPath: String,       // 输出路径
    val description: String       // 配置描述
)
```

## 🐛 故障排除

### 常见问题

1. **录音失败**
   - 检查是否授予了录音权限
   - 确认麦克风没有被其他应用占用
   - 验证设备支持所选的音频配置

2. **权限问题**
   ```bash
   adb shell setenforce 0  # 临时关闭SELinux
   adb shell chmod 755 /data/
   ```

3. **配置加载失败**
   - 检查JSON格式是否正确
   - 确认配置文件路径
   - 查看应用日志输出

4. **文件保存失败**
   - 确认输出目录的写入权限
   - 检查存储空间是否充足
   - 验证文件路径格式正确

### 调试信息

使用logcat查看详细日志：
```bash
adb logcat -s AAudioRecorder MainActivity
```

## 📊 性能指标

### 延迟测试

- **低延迟模式**: ~10-40ms
- **省电模式**: ~80-120ms
- **缓冲区大小**: 自动优化

### 支持格式

- **采样率**: 8kHz - 192kHz
- **声道数**: 1-16声道
- **位深度**: 8/16/24/32位和浮点
- **格式**: PCM WAV文件

### 性能优化建议

1. **选择合适的配置**: 根据使用场景选择最优的录音参数
2. **避免频繁切换**: 录音过程中不要切换配置
3. **及时释放资源**: 应用退出时调用 `release()` 方法
4. **监控内存使用**: 长时间录音时注意内存管理

### 性能数据

- **延迟**: 低延迟模式下可达到 10-20ms
- **CPU 使用率**: 录音时 CPU 使用率 < 5%
- **内存占用**: 基础内存占用 < 50MB

## 🤝 贡献指南

1. Fork项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开Pull Request

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- Google AAudio团队提供的优秀API
- Android NDK开发团队
- 开源社区的支持和贡献

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- 提交Issue: [GitHub Issues](https://github.com/kainan-tek/AAudioRecorder/issues)
- 邮箱: kainanos@outlook.com

---

**注意**: 本项目仅用于学习和测试目的，请遵守相关法律法规使用录音功能。