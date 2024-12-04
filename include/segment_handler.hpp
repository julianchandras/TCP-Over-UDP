#ifndef segment_handler_h
#define segment_handler_h

#include "segment.hpp"
#include "socket.hpp"
#include <iostream>
#include <vector>

using namespace std;

const uint8_t MAX_PAYLOAD_SIZE = 1460;
const uint8_t MAX_SEGMENT_SIZE = 1500;

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
    SegmentHandler(uint8_t windowSize, uint32_t currentSeqNum, uint32_t currentAckNum);

    void generateSegments();
    void setDataStream(uint8_t *dataStream, uint32_t dataSize);
    uint8_t getWindowSize();
    vector<Segment*> advanceWindow(uint8_t size);
};

#endif