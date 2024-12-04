#include "segment_handler.hpp"  
#include "utils.hpp"
#include <iostream>

using namespace std;

SegmentHandler::SegmentHandler(uint8_t windowSize, uint32_t currentSeqNum, uint32_t currentAckNum)
{
    this->windowSize = windowSize;
    this->currentSeqNum = currentSeqNum;
    this->currentAckNum = currentAckNum;
}

// sepertinya mencacah datastream mjd sekumpulan segment yang dimasukkan segmentBuffer
// asumsi segment pertama adalah segment SYN, lalu segment data
void SegmentHandler::generateSegments()
{
    uint32_t bytesProcessed = 0;
    
    this->segmentBuffer.clear();

    // putting input into a segment aka adding tcp header
    uint16_t numOfSegments = (this->dataSize / MAX_PAYLOAD_SIZE) + 1;

    for (uint16_t i = 0; i < numOfSegments; i++)
    {
        Segment seg;
        size_t offset = MAX_PAYLOAD_SIZE * i;
        size_t payloadSize = (i == numOfSegments - 1) ? (this->dataSize - offset) : MAX_PAYLOAD_SIZE;
        
        seg.sequenceNumber = this->currentSeqNum;
        seg.payload = (uint8_t *)malloc(payloadSize);

        if (seg.payload == nullptr)
        {
            cerr << "Error: Memory allocation failed for payload!" << endl;
            return;
        }

        memcpy(seg.payload, this->dataStream + offset, payloadSize);
        this->segmentBuffer.push_back(seg);

        this->currentSeqNum += offset;
    }
}

void SegmentHandler::setDataStream(uint8_t *dataStream, uint32_t dataSize)
{
    this->dataStream = dataStream;
    this->dataSize = dataSize;
    this->dataIndex = 0;
}

uint8_t SegmentHandler::getWindowSize()
{
    return this->windowSize;
}

vector<Segment*> SegmentHandler::advanceWindow(uint8_t size)
{
    vector<Segment*> segmentList;

    while (size > 0 && dataIndex < segmentBuffer.size())
    {
        Segment* segment = &segmentBuffer.at(dataIndex);
        segmentList.push_back(segment);
        dataIndex++;
        size--;
    }

    return segmentList;
}