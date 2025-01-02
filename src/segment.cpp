#include "segment.hpp"

Segment initializeSegment()
{
    Segment seg;
    memset(&seg, 0, sizeof(Segment)); // changes all of the data in memory of seg to zero
    seg.options = nullptr;
    seg.payload = nullptr;
    seg.reserved = 0; // sebenarnya seharusnya udh 0 gr2 memset, ini biar penjelas aja sih
    seg.dataOffset = 5;
    return seg;
}

Segment syn(uint32_t seqNum)
{
    Segment synSeg = initializeSegment();
    synSeg.sequenceNumber = seqNum;
    synSeg.flags.syn = 1;
    updateChecksum(synSeg, 0);
    return synSeg;
}

Segment ack(uint32_t ackNum)
{
    Segment ackSeg = initializeSegment();
    ackSeg.acknowledgementNumber = ackNum;
    ackSeg.flags.ack = 1;
    updateChecksum(ackSeg, 0);
    return ackSeg;
}

Segment synAck(uint32_t seqNum, uint32_t ackNum)
{
    Segment saSeg = initializeSegment();
    saSeg.sequenceNumber = seqNum;
    saSeg.acknowledgementNumber = ackNum;
    saSeg.flags.syn = 1;
    saSeg.flags.ack = 1;
    updateChecksum(saSeg, 0);
    return saSeg;
}

Segment fin(uint32_t seqNum)
{
    Segment finSeg = initializeSegment();
    finSeg.sequenceNumber = seqNum;
    finSeg.flags.fin = 1;
    updateChecksum(finSeg, 0);
    return finSeg;
}

Segment finAck(uint32_t seqNum, uint32_t ackNum)
{
    Segment faSeg = initializeSegment();
    faSeg.sequenceNumber = seqNum;
    faSeg.acknowledgementNumber = ackNum;
    faSeg.flags.fin = 1;
    faSeg.flags.ack = 1;
    updateChecksum(faSeg, 0);
    return faSeg;
}

uint8_t *calculateChecksum(Segment segment, size_t payloadSize)
{
    uint32_t sum = 0;

    sum += segment.sourcePort;
    sum += segment.destPort;

    sum += (segment.sequenceNumber >> 16) & 0xFFFF;
    sum += segment.sequenceNumber & 0xFFFF;
    sum += (segment.acknowledgementNumber >> 16) & 0xFFFF;
    sum += segment.acknowledgementNumber & 0xFFFF;

    sum += (segment.dataOffset << 12) | (segment.reserved << 8) | flagsToByte(segment);

    sum += segment.window;
    sum += segment.urgentPointer;

    if (segment.payload)
    {
        for (size_t i = 0; i < payloadSize; i += 2)
        {
            uint16_t word = (segment.payload[i] << 8) | (i + 1 < payloadSize ? segment.payload[i + 1] : 0);
            sum += word;
        }
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t checksum = ~static_cast<uint16_t>(sum);
    auto *checksumBytes = new uint8_t[2];
    checksumBytes[0] = (checksum >> 8) & 0xFF;
    checksumBytes[1] = checksum & 0xFF;

    return checksumBytes;
}

void updateChecksum(Segment &segment, size_t payloadSize)
{
    uint8_t *checksumBytes = calculateChecksum(segment, payloadSize);
    segment.checkSum = (static_cast<uint16_t>(checksumBytes[0]) << 8) | checksumBytes[1];
    delete[] checksumBytes;
}

bool isValidChecksum(Segment segment, size_t payloadSize)
{
    uint8_t *checksumBytes = calculateChecksum(segment, payloadSize);
    uint16_t checkSum = ~((static_cast<uint16_t>(checksumBytes[0]) << 8) | checksumBytes[1]);
    delete[] checksumBytes;
    
    return checkSum + segment.checkSum == 0xFFFF;
}


uint8_t flagsToByte(Segment segment)
{
    return (segment.flags.fin | (segment.flags.syn << 1) | (segment.flags.rst << 2) |
            (segment.flags.psh << 3) | (segment.flags.ack << 4) | (segment.flags.urg << 5) |
            (segment.flags.ece << 6) | (segment.flags.cwr << 7));
}

