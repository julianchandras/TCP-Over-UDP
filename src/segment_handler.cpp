#include "segment_handler.hpp"

using namespace std;

// sepertinya mencacah datastream mjd sekumpulan segment yang dimasukkan segmentBuffer
// asumsi segment pertama adalah segment SYN, lalu segment data
void SegmentHandler::generateSegments()
{
    uint32_t bytesProcessed = 0;
    uint32_t sequenceNumber = 0;
    this->segmentBuffer.clear();
}

void SegmentHandler::setDataStream(uint8_t *dataStream, uint32_t dataSize)
{
    this->dataStream = dataStream;
    this->dataSize = dataSize;
}

uint8_t SegmentHandler::getWindowSize()
{
}

Segment *SegmentHandler::advanceWindow(uint8_t size)
{
}