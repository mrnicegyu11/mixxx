#ifndef ENGINE_CACHINGREADERCHUNK_H
#define ENGINE_CACHINGREADERCHUNK_H

#include "sources/audiosource.h"

// A Chunk is a memory-resident section of audio that has been cached.
// Each chunk holds a fixed number kFrames of frames with samples for
// kChannels.
//
// The class is not thread-safe although it is shared between CachingReader
// and CachingReaderWorker! A lock-free FIFO ensures that only a single
// thread has exclusive access on each chunk. This abstract base class
// is available for both the worker thread and the cache.
//
// This is the common (abstract) base class for both the cache (as the owner)
// and the worker.
class CachingReaderChunk {
public:
    static const mixxx::AudioSignal::ChannelCount kChannels;
    static const SINT kFrames;
    static const SINT kSamples;

    // Converts frames to samples
    inline static SINT frames2samples(SINT frames) {
        return frames * kChannels;
    }
    // Converts samples to frames
    inline static SINT samples2frames(SINT samples) {
        DEBUG_ASSERT(0 == (samples % kChannels));
        return samples / kChannels;
    }

    // Returns the corresponding chunk index for a frame index
    inline static SINT indexForFrame(
            /*const mixxx::AudioSourcePointer& pAudioSource,*/
            SINT frameIndex) {
        //DEBUG_ASSERT(pAudioSource->frameIndexRange().contains(frameIndex));
        const SINT frameIndexOffset = frameIndex /*- pAudioSource->frameIndexMin()*/;
        return frameIndexOffset / kFrames;
    }

    // Disable copy and move constructors
    CachingReaderChunk(const CachingReaderChunk&) = delete;
    CachingReaderChunk(CachingReaderChunk&&) = delete;

    SINT getIndex() const {
        return m_index;
    }

    // Frame index range of this chunk for the given audio source.
    mixxx::IndexRange frameIndexRange(
            const mixxx::AudioSourcePointer& pAudioSource) const;

    // Read sample frames from the audio source and return the
    // range of frames that have been read.
    mixxx::IndexRange bufferSampleFrames(
            const mixxx::AudioSourcePointer& pAudioSource,
            SampleBuffer::WritableSlice tempOutputBuffer);

    mixxx::IndexRange readBufferedSampleFrames(
            CSAMPLE* sampleBuffer,
            const mixxx::IndexRange& frameIndexRange) const;
    mixxx::IndexRange readBufferedSampleFramesReverse(
            CSAMPLE* reverseSampleBuffer,
            const mixxx::IndexRange& frameIndexRange) const;

protected:
    explicit CachingReaderChunk(
            SampleBuffer::WritableSlice sampleBuffer);
    virtual ~CachingReaderChunk();

    void init(SINT index);

private:
    SINT frameIndexOffset() const {
        return m_index * kFrames;
    }

    SINT m_index;

    // The worker thread will fill the sample buffer and
    // set the corresponding frame index range.
    SampleBuffer::WritableSlice m_sampleBuffer;
    mixxx::IndexRange m_bufferedFrameIndexRange;
};

// This derived class is only accessible for the cache as the owner,
// but not the worker thread. The state READ_PENDING indicates that
// the worker thread is in control.
class CachingReaderChunkForOwner: public CachingReaderChunk {
public:
    explicit CachingReaderChunkForOwner(
            SampleBuffer::WritableSlice sampleBuffer);
    ~CachingReaderChunkForOwner() override;

    void init(SINT index);
    void free();

    enum State {
        FREE,
        READY,
        READ_PENDING
    };

    State getState() const {
        return m_state;
    }

    // The state is controlled by the cache as the owner of each chunk!
    void giveToWorker() {
        DEBUG_ASSERT(READY == m_state);
        m_state = READ_PENDING;
    }
    void takeFromWorker() {
        DEBUG_ASSERT(READ_PENDING == m_state);
        m_state = READY;
    }

    // Inserts a chunk into the double-linked list before the
    // given chunk. If the list is currently empty simply pass
    // pBefore = nullptr. Please note that if pBefore points to
    // the head of the current list this chunk becomes the new
    // head of the list.
    void insertIntoListBefore(
            CachingReaderChunkForOwner* pBefore);
    // Removes a chunk from the double-linked list and optionally
    // adjusts head/tail pointers. Pass ppHead/ppTail = nullptr if
    // you prefer to adjust those pointers manually.
    void removeFromList(
            CachingReaderChunkForOwner** ppHead,
            CachingReaderChunkForOwner** ppTail);

private:
    State m_state;

    CachingReaderChunkForOwner* m_pPrev; // previous item in double-linked list
    CachingReaderChunkForOwner* m_pNext; // next item in double-linked list
};


#endif // ENGINE_CACHINGREADERCHUNK_H
