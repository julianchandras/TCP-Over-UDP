#ifndef segment_handler_h
#define segment_handler_h

#include "segment.hpp"
#include <iostream>
#include <vector>

const uint16_t MAX_PAYLOAD_SIZE = 1460;
const uint16_t MAX_SEGMENT_SIZE = 1480;

class SegmentHandler
{
private:
    uint8_t windowSize;
    uint32_t currentSeqNum;
    uint32_t currentAckNum;
    void *dataStream;
    std::vector<Segment> segmentBuffer;
    uint32_t dataSize;
    uint32_t dataIndex;

public:
    SegmentHandler(uint8_t windowSize, uint32_t currentSeqNum, uint32_t currentAckNum);

    void generateSegments();
    void setDataStream(uint8_t *dataStream, uint32_t dataSize);
    uint8_t getWindowSize();
    uint32_t getCurrentSeqNum();
    std::vector<Segment *> advanceWindow(uint8_t size);

    void appendSegmentBuffer(Segment *Seg);
    void getDatastream(uint8_t *dataStream, uint32_t bufferSize);

    // cuman buat server
    std::vector<Segment> ackChecker;
};

#endif
