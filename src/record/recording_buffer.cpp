/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include "record/recording_buffer.h"

void recording_buffer_s::initialize(const size_t frameCapacity)
{
    // Frames are in 24-bit color.
    const unsigned maxFrameSize = (this->maxWidth * this->maxHeight * 3);

    this->frameData.resize(frameCapacity);
    this->isFrameInUse.resize(frameCapacity);

    for (auto &buffer: this->frameData)
    {
        buffer.allocate(maxFrameSize, "Video recording frame buffer.");
    }

    this->scratchBuffer.allocate(maxFrameSize, "Video recording scratch buffer.");

    this->reset();

    return;
}

void recording_buffer_s::release(void)
{
    for (auto &buffer: this->frameData)
    {
        buffer.release();
    }

    this->scratchBuffer.release();

    return;
}

bool recording_buffer_s::empty(void)
{
    return this->frameQueue.empty();
}

bool recording_buffer_s::full(void)
{
    return (this->frameQueue.size() >= this->frameData.size());
}

heap_mem<u8>* recording_buffer_s::push(void)
{
    k_assert(!this->full(), "Attempting to push to a queue that is full.");

    for (size_t i = 0; i < this->frameData.size(); i++)
    {
        if (!this->isFrameInUse.at(i))
        {
            const auto bufferPtr = &this->frameData.at(i);

            this->isFrameInUse.at(i) = true;
            this->frameQueue.push(i);

            return bufferPtr;
        }
    }

    k_assert(0, "Could not find a free buffer.");
    return nullptr;
}

heap_mem<u8>* recording_buffer_s::pop(void)
{
    k_assert(this->frameQueue.size(), "Attempting to pop an empty queue.");

    const size_t bufferIdx = this->frameQueue.front();
    const auto bufferPtr = &this->frameData.at(bufferIdx);

    this->frameQueue.pop();
    this->isFrameInUse.at(bufferIdx) = false;

    return bufferPtr;
}

void recording_buffer_s::reset(void)
{
    std::fill(this->isFrameInUse.begin(),
              this->isFrameInUse.end(),
              false);

    this->frameQueue = std::queue<size_t>();

    return;
}

unsigned recording_buffer_s::usage(void)
{
    const unsigned percentInUse = unsigned((this->frameQueue.size() / double(this->frameData.size())) * 100);

    return percentInUse;
}
