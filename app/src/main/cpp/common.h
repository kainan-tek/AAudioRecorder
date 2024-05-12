//
// Created by kaina on 2024/5/1.
//

#ifndef AAUDIORECORDER_LOG_H
#define AAUDIORECORDER_LOG_H

#include <android/log.h>

#define LOG_TAG  "AAudioRecorder"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#endif //AAUDIORECORDER_LOG_H
