#include "utils.hpp"
#include <stdlib.h>
#include <iostream>

using namespace std;

uint8_t *serializeSegment(Segment *toBeSent, size_t optionsLen, size_t payloadLen)
{
    size_t total_size = BASE_SEGMENT_SIZE + optionsLen + payloadLen;

    uint8_t *buffer = (uint8_t *) malloc(total_size);
    if (!buffer)
    {
        perror("malloc");
    }
    // masukkin isi segment tanpa option sama payload ke buffer
    memcpy(buffer, toBeSent, BASE_SEGMENT_SIZE);
    // masukkin option
    memcpy(buffer + BASE_SEGMENT_SIZE, toBeSent->options, optionsLen);
    // masukkin payload
    memcpy(buffer + BASE_SEGMENT_SIZE + optionsLen, toBeSent->payload, payloadLen);
    return buffer;
}

void deserializeToSegment(Segment *segment, uint8_t *buffer, size_t nBytes)
{
    // copies only the header without options
    memcpy(segment, buffer, BASE_SEGMENT_SIZE);
    size_t optionsLen = 0; // asumsi ga ada options, srsly buat apa
    size_t payloadLen = nBytes - BASE_SEGMENT_SIZE - optionsLen;
    // Allocate and copy payload
    segment->payload = (uint8_t *)malloc(payloadLen);
    memcpy(segment->payload, buffer + BASE_SEGMENT_SIZE + optionsLen, payloadLen);
}

void printSegment(const Segment &seg, size_t payloadLen)
{
    cout << "Segment Header Information:" << endl;
    cout << "  Source Port: " << seg.sourcePort << endl;
    cout << "  Destination Port: " << seg.destPort << endl;
    cout << "  Sequence Number: " << seg.sequenceNumber << endl;
    cout << "  Acknowledgment Number: " << seg.acknowledgementNumber << endl;
    cout << "  Data Offset: " << seg.dataOffset << endl;
    cout << "  Flags: " << endl;
    cout << "    CWR: " << seg.flags.cwr << endl;
    cout << "    ECE: " << seg.flags.ece << endl;
    cout << "    URG: " << seg.flags.urg << endl;
    cout << "    ACK: " << seg.flags.ack << endl;
    cout << "    PSH: " << seg.flags.psh << endl;
    cout << "    RST: " << seg.flags.rst << endl;
    cout << "    SYN: " << seg.flags.syn << endl;
    cout << "    FIN: " << seg.flags.fin << endl;
    cout << "  Window: " << seg.window << endl;
    cout << "  Checksum: " << seg.checkSum << endl;
    cout << "  Urgent Pointer: " << seg.urgentPointer << endl;

    cout << "\nPayload (" << payloadLen << " bytes):" << endl;
    cout << "  ";
    for (size_t i = 0; i < payloadLen; ++i)
    {
        cout << seg.payload[i];
    }
    cout << endl;
}