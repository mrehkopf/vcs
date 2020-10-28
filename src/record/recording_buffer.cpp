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
        buffer.alloc(maxFrameSize);
    }

    this->scratchBuffer.alloc(maxFrameSize);

    this->reset();

    return;
}

void recording_buffer_s::release(void)
{
    for (auto &buffer: this->frameData)
    {
        buffer.release_memory();
    }

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

heap_bytes_s<u8>* recording_buffer_s::push(void)
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

heap_bytes_s<u8>* recording_buffer_s::pop(void)
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

    return;
}

unsigned recording_buffer_s::usage(void)
{
    const unsigned percentInUse = unsigned((this->frameQueue.size() / double(this->frameData.size())) * 100);

    return percentInUse;
}