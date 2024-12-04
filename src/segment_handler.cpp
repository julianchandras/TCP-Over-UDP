#include "segment_handler.hpp"  
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
    uint32_t sequenceNumber = this->currentSeqNum;
    
    this->segmentBuffer.clear();

    // putting input into a segment aka adding tcp header
    uint16_t numOfSegments = (this->dataSize / MAX_PAYLOAD_SIZE) + 1;

    for (uint16_t i = 0; i < numOfSegments; i++)
    {
        Segment temp;
        size_t offset = MAX_PAYLOAD_SIZE * i;
        temp.sequenceNumber = sequenceNumber + offset;
        size_t copySize = (i == numOfSegments - 1) ? (this->dataSize - offset) : MAX_PAYLOAD_SIZE;
        temp.payload = (uint8_t *)malloc(copySize);

        if (temp.payload == nullptr)
        {
            cerr << "Error: Memory allocation failed for payload!" << endl;
            return;
        }

        memcpy(temp.payload, this->dataStream + offset, copySize);

        this->segmentBuffer.push_back(temp);
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