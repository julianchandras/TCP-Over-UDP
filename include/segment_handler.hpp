#ifndef segment_handler_h
#define segment_handler_h

#include "segment.hpp"
#include <vector>

using namespace std;

class SegmentHandler
{
private:
    uint8_t windowSize;
    uint32_t currentSeqNum;
    uint32_t currentAckNum;
    void *dataStream;
    vector<Segment> segmentBuffer;
    uint32_t dataSize;
    uint32_t dataIndex;
    void generateSegments();

public:
    void setDataStream(uint8_t *dataStream, uint32_t dataSize);
    uint8_t getWindowSize();
    Segment *advanceWindow(uint8_t size);
};

#endif