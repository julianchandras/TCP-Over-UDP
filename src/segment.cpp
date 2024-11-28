#include "segment.hpp"

Segment initializeSegment()
{
    Segment seg;
    memset(&seg, 0, sizeof(Segment)); // changes all of the data in memory of seg to zero
    seg.options = nullptr;
    seg.payload = nullptr;
    seg.reserved = 0;     // sebenarnya seharusnya udh 0 gr2 memset, ini biar penjelas aja sih
    seg.data_offset = 20; // minimum is 20 bytes
    return seg;
}

Segment syn(uint32_t seqNum)
{
    Segment synSeg = initializeSegment();
    synSeg.sequenceNumber = seqNum;
    synSeg.flags.syn = SYN_FLAG;
    return synSeg;
}

Segment ack(uint32_t seqNum, uint32_t ackNum)
{
    Segment ackSeg = initializeSegment();
    ackSeg.sequenceNumber = seqNum;
    ackSeg.acknowledgementNumber = ackNum;
    ackSeg.flags.ack = ACK_FLAG;
    return ackSeg;
}

Segment synAck(uint32_t seqNum)
{
    Segment saSeg = initializeSegment();
    saSeg.sequenceNumber = seqNum;
    saSeg.flags.syn = SYN_FLAG;
    saSeg.flags.ack = ACK_FLAG;
    return saSeg;
}

Segment fin()
{
    Segment finSeg = initializeSegment();
    finSeg.flags.fin = FIN_FLAG;
    return finSeg;
}

Segment finAck()
{
    Segment faSeg = initializeSegment();
    faSeg.flags.fin = FIN_FLAG;
    faSeg.flags.ack = ACK_FLAG;
    return faSeg;
}

uint16_t computeChecksum(const uint8_t *data, size_t length) {
    uint32_t sum = 0;
    
    for (size_t i = 0; i < length; i += 2) {
        uint16_t word = (data[i] << 8) | (i + 1 < length ? data[i + 1] : 0);
        sum += word;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

uint8_t *calculateChecksum(Segment segment)
{
    uint32_t sum = 0;

    sum += segment.sourcePort;
    sum += segment.destPort;

    sum += (segment.sequenceNumber >> 16) & 0xFFFF;
    sum += segment.sequenceNumber & 0xFFFF;
    sum += (segment.acknowledgementNumber >> 16) & 0xFFFF;
    sum += segment.acknowledgementNumber & 0xFFFF;

    sum += (segment.data_offset << 12) | (segment.reserved << 8) | flagsToByte(segment);

    sum += segment.window;
    sum += segment.urgentPointer;

    if (segment.payload)
    {
        size_t payloadSize = strlen(reinterpret_cast<const char *>(segment.payload));
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

Segment updateChecksum(Segment segment)
{
    uint8_t *checksumBytes = calculateChecksum(segment);
    segment.checkSum = (checksumBytes[0] << 8) | checksumBytes[1];
    delete[] checksumBytes;
    return segment;
}

bool isValidChecksum(Segment segment)
{
    uint8_t *checksumBytes = calculateChecksum(segment);
    uint16_t checkSum = (checksumBytes[0] << 8) | checksumBytes[1];
    delete[] checksumBytes;
    return checkSum+segment.checkSum == 0xFFFF;
}

uint8_t flagsToByte(Segment segment)
{
    return (segment.flags.fin | (segment.flags.syn << 1) | (segment.flags.rst << 2) |
            (segment.flags.psh << 3) | (segment.flags.ack << 4) | (segment.flags.urg << 5) |
            (segment.flags.ece << 6) | (segment.flags.cwr << 7));
}