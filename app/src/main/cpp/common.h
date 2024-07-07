//
// Created by kaina on 2024/5/1.
//

#ifndef AAUDIORECORDER_LOG_H
#define AAUDIORECORDER_LOG_H

#include <android/log.h>

#define LOG_TAG "AAudioRecorder"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define MAX_DATA_SIZE (1 * 1024 * 1024 * 1024)

#define ENABLE_CALLBACK 1
#define USE_WAV_HEADER 1

#ifdef USE_WAV_HEADER
// WAV file header
struct WAVHeader; // Defined in wav-header.cpp
bool writeWAVHeader(std::ofstream &outFile, uint32_t numSamples, uint32_t sampleRate, uint32_t numChannels, uint32_t bitsPerSample);
// bool readWAVHeader(const std::string &filename, WAVHeader &header);
void UpdateSizes(std::ofstream &outfile, uint32_t data_chunk_size);
#endif // USE_WAV_HEADER

#endif // AAUDIORECORDER_LOG_H
