#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <AudioToolbox/AudioToolbox.h>

class WavPlayer {
private:
    std::vector<int16_t> audioData;
    uint32_t sampleRate;
    uint16_t numChannels;
    AudioQueueRef audioQueue;
    AudioQueueBufferRef audioBuffer;

public:
    WavPlayer(const std::string& fileName) {
        loadWavFile(fileName);
    }

    void play() {
        if (audioData.empty()) {
            std::cerr << "Error: No audio data loaded\n";
            return;
        }

        AudioStreamBasicDescription audioFormat;
        audioFormat.mSampleRate = sampleRate;
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mChannelsPerFrame = numChannels;
        audioFormat.mBitsPerChannel = sizeof(int16_t) * 8;
        audioFormat.mBytesPerFrame = sizeof(int16_t) * numChannels;
        audioFormat.mBytesPerPacket = audioFormat.mBytesPerFrame * audioFormat.mFramesPerPacket;

        // Create audio queue
        OSStatus status = AudioQueueNewOutput(&audioFormat, callback, this, nullptr, nullptr, 0, &audioQueue);
        if (status != noErr) {
            std::cerr << "Error: Unable to create audio queue\n";
            return;
        }

        // Allocate and enqueue buffers
        for (int i = 0; i < 3; ++i) {
            AudioQueueAllocateBuffer(audioQueue, sizeof(int16_t) * audioData.size(), &audioBuffer);
            memcpy(audioBuffer->mAudioData, audioData.data(), sizeof(int16_t) * audioData.size());
            audioBuffer->mAudioDataByteSize = sizeof(int16_t) * audioData.size();
            AudioQueueEnqueueBuffer(audioQueue, audioBuffer, 0, nullptr);
        }

        // Start audio queue
        AudioQueueStart(audioQueue, nullptr);

        // Run the loop to keep the program alive while audio is playing
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10, false);

        // Cleanup
        AudioQueueFlush(audioQueue);
        AudioQueueDispose(audioQueue, true);
    }

private:
    static void callback(void* userData, AudioQueueRef, AudioQueueBufferRef) {
        // This callback is necessary for the audio queue to work
    }

    void loadWavFile(const std::string& fileName) {
        std::ifstream file(fileName, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Error: Unable to open the file " << fileName << "\n";
            return;
        }

        // Read WAV file header
        char header[44];
        file.read(header, sizeof(header));

        // Check if the file is a valid WAV file
        if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
            std::cerr << "Error: Not a valid WAV file\n";
            return;
        }

        // Extract sample rate and number of channels from the header
        sampleRate = *reinterpret_cast<uint32_t*>(header + 24);
        numChannels = *reinterpret_cast<uint16_t*>(header + 22);

        // Read audio data
        const size_t dataSize = *reinterpret_cast<uint32_t*>(header + 40);
        audioData.resize(dataSize / sizeof(int16_t));

        file.read(reinterpret_cast<char*>(audioData.data()), dataSize);

        file.close();
    }
};

int main() {
    // Create an instance of WavPlayer
    WavPlayer player("song.wav");

    // Play the audio
    player.play();

    return 0;
}
