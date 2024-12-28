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
void SegmentHandler::generateSegments()
{
    // uint32_t bytesProcessed = 0;

    this->segmentBuffer.clear();

    // putting input into a segment aka adding tcp header
    uint16_t numOfSegments = (this->dataSize / MAX_PAYLOAD_SIZE) + 1;

    for (uint16_t i = 0; i < numOfSegments; i++)
    {
        Segment seg = initializeSegment();
        size_t offset = MAX_PAYLOAD_SIZE * i;

        size_t payloadSize;

        if (i == numOfSegments - 1)
        {
            payloadSize = this->dataSize - offset;
        }
        else
        {
            payloadSize = MAX_PAYLOAD_SIZE;
        }

        seg.sequenceNumber = this->currentSeqNum;
        seg.payload = (uint8_t *)malloc(payloadSize);

        if (seg.payload == nullptr)
        {
            cerr << "Error: Memory allocation failed for payload!" << endl;
            return;
        }

        memcpy(seg.payload, this->dataStream + offset, payloadSize);
        updateChecksum(seg, payloadSize);
        this->segmentBuffer.push_back(seg);
        this->currentSeqNum += payloadSize;
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

uint32_t SegmentHandler::getCurrentSeqNum()
{
    return this->currentSeqNum;
}

size_t SegmentHandler::getSegmentBufferSize()
{
    return this->segmentBuffer.size();
}

void SegmentHandler::advanceWindow(uint8_t size, std::vector<Segment *> *window)
{
    // cout << "segment buffer total size: " << segmentBuffer.size();
    if (segmentBuffer.size() < size)
    {
        size = segmentBuffer.size();
    }
    while (size > 0 && dataIndex < segmentBuffer.size())
    {
        Segment *segment = &segmentBuffer.at(dataIndex);
        window->push_back(segment);
        dataIndex++;
        size--;
    }
}

void SegmentHandler::appendSegmentBuffer(Segment *seg)
{
    this->segmentBuffer.push_back(*seg);
}

void SegmentHandler::getDatastream(std::vector<uint8_t> &dataStream)
{
    if (this->segmentBuffer.empty())
    {
        throw runtime_error("No segments to reconstruct the data stream!");
    }

    dataStream.clear();

    auto lastIter = segmentBuffer.end() - 1;

    for (auto iter = segmentBuffer.begin(); iter != segmentBuffer.end(); ++iter)
    {
        const Segment &seg = *iter;
        
        dataStream.insert(dataStream.end(), seg.payload, seg.payload + MAX_PAYLOAD_SIZE);
    }
}

