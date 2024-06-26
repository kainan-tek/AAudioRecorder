//
// Created by kaina on 2024/6/23.
//

#include <cstdint>
#include <fstream>
#include "common.h"

struct WAVHeader
{
    // WAV header size is 44 bytes
    // format = RIFF + fmt + data
    char riffID[4];         // "RIFF"
    uint32_t riffSize;      // 36 + (numSamples * numChannels * bytesPerSample)
    char waveID[4];         // "WAVE"
    char fmtID[4];          // "fmt "
    uint32_t fmtSize;       // 16 for PCM; 18 for IEEE float
    uint16_t audioFormat;   // 1 for PCM; 3 for IEEE float
    uint16_t numChannels;   // 1 for mono; 2 for stereo
    uint32_t sampleRate;    // sample rate
    uint32_t byteRate;      // sampleRate * numChannels * bytesPerSample
    uint16_t blockAlign;    // numChannels * bytesPerSample
    uint16_t bitsPerSample; // 8 for 8-bit; 16 for 16-bit; 32 for 32-bit
    char dataID[4];         // "data"
    uint32_t dataSize;      // numSamples * numChannels * bytesPerSample

    /**
     * Writes the WAV file header to the specified output stream.
     *
     * @param out The output stream to write the header to.
     *
     * @throws None
     */
    void write(std::ofstream &out)
    {
        out.write(riffID, 4);                 // "RIFF"
        out.write((char *)&riffSize, 4);      // 36 + (numSamples * numChannels * bytesPerSample)
        out.write(waveID, 4);                 // "WAVE"
        out.write(fmtID, 4);                  // "fmt "
        out.write((char *)&fmtSize, 4);       // 16 for PCM; 18 for IEEE float
        out.write((char *)&audioFormat, 2);   // 1 for PCM; 3 for IEEE float
        out.write((char *)&numChannels, 2);   // 1 for mono; 2 for stereo
        out.write((char *)&sampleRate, 4);    // sample rate
        out.write((char *)&byteRate, 4);      // sampleRate * numChannels * bytesPerSample
        out.write((char *)&blockAlign, 2);    // numChannels * bytesPerSample
        out.write((char *)&bitsPerSample, 2); // 8 for 8-bit; 16 for 16-bit; 32 for 32-bit
        out.write(dataID, 4);                 // "data"
        out.write((char *)&dataSize, 4);      // numSamples * numChannels * bytesPerSample
    }

    /**
     * Reads the WAV file header from the specified input stream.
     *
     * @param in The input stream to read the header from.
     *
     * @throws None
     */
    void read(std::ifstream &in)
    {
        in.read(riffID, 4);                 // "RIFF"
        in.read((char *)&riffSize, 4);      // 36 + (numSamples * numChannels * bytesPerSample)
        in.read(waveID, 4);                 // "WAVE"
        in.read(fmtID, 4);                  // "fmt "
        in.read((char *)&fmtSize, 4);       // 16 for PCM; 18 for IEEE float
        in.read((char *)&audioFormat, 2);   // 1 for PCM; 3 for IEEE float
        in.read((char *)&numChannels, 2);   // 1 for mono; 2 for stereo
        in.read((char *)&sampleRate, 4);    // sample rate
        in.read((char *)&byteRate, 4);      // sampleRate * numChannels * bytesPerSample
        in.read((char *)&blockAlign, 2);    // numChannels * bytesPerSample
        in.read((char *)&bitsPerSample, 2); // 8 for 8-bit; 16 for 16-bit; 32 for 32-bit
        in.read(dataID, 4);                 // "data"
        in.read((char *)&dataSize, 4);      // numSamples * numChannels * bytesPerSample
    }

    /** Prints the WAV file header. */
    void print()
    {
        ALOGI("RiffID: %s\n", riffID);
        ALOGI("RiffSize: %d\n", riffSize);
        ALOGI("WaveID: %s\n", waveID);
        ALOGI("FmtID: %s\n", fmtID);
        ALOGI("FmtSize: %d\n", fmtSize);
        ALOGI("AudioFormat: %d\n", audioFormat);
        ALOGI("NumChannels: %d\n", numChannels);
        ALOGI("SampleRate: %d\n", sampleRate);
        ALOGI("ByteRate: %d\n", byteRate);
        ALOGI("BlockAlign: %d\n", blockAlign);
        ALOGI("BitsPerSample: %d\n", bitsPerSample);
        ALOGI("DataID: %s\n", dataID);
        ALOGI("DataSize: %d\n", dataSize);
    }
};

/**
 * Writes the WAV file header to the specified output stream.
 *
 * @param outFile The output stream to write the header to.
 * @param numSamples The number of samples in the audio data.
 * @param sampleRate The sample rate of the audio data.
 * @param numChannels The number of channels in the audio data.
 * @param bitsPerSample The number of bits per sample in the audio data.
 *
 * @return True if the header was written successfully, false otherwise.
 */
bool writeWAVHeader(std::ofstream &outFile, uint32_t numSamples, uint32_t sampleRate, uint32_t numChannels, uint32_t bitsPerSample)
{
    if (!outFile.is_open())
    {
        return false;
    }

    WAVHeader header{};
    header.riffID[0] = 'R';
    header.riffID[1] = 'I';
    header.riffID[2] = 'F';
    header.riffID[3] = 'F';

    header.riffSize = 0;
    header.waveID[0] = 'W';
    header.waveID[1] = 'A';
    header.waveID[2] = 'V';
    header.waveID[3] = 'E';

    header.fmtID[0] = 'f';
    header.fmtID[1] = 'm';
    header.fmtID[2] = 't';
    header.fmtID[3] = ' ';

    header.fmtSize = 16;
    header.audioFormat = 1;
    header.numChannels = numChannels;
    header.sampleRate = sampleRate;

    header.byteRate = sampleRate * numChannels * bitsPerSample / 8;
    header.blockAlign = numChannels * bitsPerSample / 8;
    header.bitsPerSample = bitsPerSample;

    header.dataID[0] = 'd';
    header.dataID[1] = 'a';
    header.dataID[2] = 't';
    header.dataID[3] = 'a';
    header.dataSize = numSamples * numChannels * bitsPerSample / 8;

    header.write(outFile);

    return true;
}

/**
 * Reads the WAV file header from the specified input stream.
 *
 * @param inFile The input stream to read the header from.
 * @param header The WAVHeader object to store the header in.
 *
 * @return True if the header was read successfully, false otherwise.
 */
bool readWAVHeader(const std::string &filename, WAVHeader &header)
{
    std::ifstream in(filename, std::ios::binary | std::ios::in);
    if (!in.is_open())
    {
        return false;
    }
    header.read(in);
    if (in.is_open())
    {
        in.close();
    }
    return true;
}

/**
 * Updates the sizes of the RIFF and data chunks in the WAV file.
 *
 * @param outfile The output stream to write the updated header to.
 * @param data_chunk_size The size of the data chunk in bytes.
 */
void UpdateSizes(std::ofstream &outfile, uint32_t data_chunk_size)
{
    // record current position
    std::streampos current_position = outfile.tellp();

    // calculates RIFF chunk size and data chunk size
    uint32_t riff_size = 4 + (8 + 16) + (8 + data_chunk_size);
    uint32_t data_size = data_chunk_size;

    // move to riff size field and update
    outfile.seekp(4, std::ios::beg);
    outfile.write(reinterpret_cast<const char *>(&riff_size), sizeof(riff_size));

    // move to data size field and update
    outfile.seekp(40, std::ios::beg);
    outfile.write(reinterpret_cast<const char *>(&data_size), sizeof(data_size));

    // return to record position
    outfile.seekp(current_position);
}
