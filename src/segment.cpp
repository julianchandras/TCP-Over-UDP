#include "include/segment.hpp"

Segment initializeSegment()
{
    Segment seg;
    memset(&seg, 0, sizeof(Segment)); // changes all of the data in memory of seg to zero
    seg.options = nullptr;
    seg.payload = nullptr;
    seg.reserved = 0;     // sebenarnya seharusnya udh 0 gr2 memset, ini biar penjelas aja sih
    seg.data_offset = 20; // minimun is 20 bytes
    return seg;
}

/**
 * Generate Segment that contain SYN packet
 */
Segment syn(uint32_t seqNum)
{
    Segment synSeg = initializeSegment();
    synSeg.sequenceNumber = seqNum;
    synSeg.flags.syn = SYN_FLAG;
    return synSeg;
}

/**
 * Generate Segment that contain ACK packet
 */
Segment ack(uint32_t seqNum, uint32_t ackNum)
{
    Segment ackSeg = initializeSegment();
    ackSeg.sequenceNumber = seqNum;
    ackSeg.acknowledgementNumber = ackNum;
    ackSeg.flags.ack = ACK_FLAG;
    return ackSeg;
}

/**
 * Generate Segment that contain SYN-ACK packet
 */
Segment synAck(uint32_t seqNum)
{
    Segment saSeg = initializeSegment();
    saSeg.sequenceNumber = seqNum;
    saSeg.flags.syn = SYN_FLAG;
    saSeg.flags.ack = ACK_FLAG;
    return saSeg;
}

/**
 * Generate Segment that contain FIN packet
 */
Segment fin()
{
    Segment finSeg = initializeSegment();
    finSeg.flags.fin = FIN_FLAG;
    return finSeg;
}

/**
 * Generate Segment that contain FIN-ACK packet
 */
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
