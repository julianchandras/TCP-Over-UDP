#include "segment_handler.hpp"
#include "segment.hpp"
#include "utils.hpp"
#include <iostream>

using namespace std;

void SegmentHandler::sendData(TCPSocket *socket, string input)
{
    generateSegments(input, socket->getRandomSeqNum());

    vector<Segment>::iterator myItr;

    uint32_t i = 1;

    for (myItr = this->segmentBuffer.begin(); myItr != this->segmentBuffer.end(); myItr++)
    {
        cout << myItr->payload << " " << endl
             << i << endl;
        uint8_t *buffer = serializeSegment(&(*myItr), 0, 1460);
        socket->send("127.0.0.1", 5679, buffer, 1460);
        i++;
    }

    cout << "SEND DATA" << endl;
}

// sepertinya mencacah datastream mjd sekumpulan segment yang dimasukkan segmentBuffer
// asumsi segment pertama adalah segment SYN, lalu segment data
void SegmentHandler::generateSegments(string input, uint32_t initialSeqNum)
{
    uint32_t bytesProcessed = 0;
    uint32_t sequenceNumber = 0;
    this->segmentBuffer.clear();

    const char *inpBuf = input.c_str();

    // putting input into a segment aka adding tcp header
    uint16_t numOfSegments = (input.length() / 1460) + 1;

    for (uint16_t i = 0; i < numOfSegments; i++)
    {
        Segment temp;
        size_t offset = 1460 * i;
        temp.sequenceNumber = initialSeqNum + offset;
        size_t copySize = (i == numOfSegments - 1) ? (input.length() - offset) : 1460;
        temp.payload = (uint8_t *)malloc(copySize);

        if (temp.payload == nullptr)
        {
            cerr << "Error: Memory allocation failed for payload!" << endl;
            return;
        }

        memcpy(temp.payload, inpBuf + offset, copySize);

        this->segmentBuffer.push_back(temp);
    }
}

void SegmentHandler::setDataStream(uint8_t *dataStream, uint32_t dataSize)
{
    this->dataStream = dataStream;
    this->dataSize = dataSize;
}

uint8_t SegmentHandler::getWindowSize()
{
    uint8_t temp = 5;
    return temp;
}

Segment *SegmentHandler::advanceWindow(uint8_t size)
{
    Segment *segment;
    return segment;
}