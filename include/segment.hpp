#ifndef segment_h
#define segment_h

#include <cstdint>
#include <stdio.h>
#include <string.h>

struct Segment
{
    uint16_t sourcePort : 16;
    uint16_t destPort : 16;
    // todo continue
    uint32_t sequenceNumber : 32;
    uint32_t acknowledgementNumber : 32;
    struct
    {
        uint8_t dataOffset : 4; // the unit for dataOffset is word (per 4 bytes)
        uint8_t reserved : 4;
    };

    struct
    {
        uint8_t cwr : 1;
        // todo continue ...
        uint8_t ece : 1;
        uint8_t urg : 1;
        uint8_t ack : 1;
        uint8_t psh : 1;
        uint8_t rst : 1;
        uint8_t syn : 1;
        uint8_t fin : 1;
    } flags;

    uint16_t window : 16;
    // todo continue
    uint16_t checkSum : 16;
    uint16_t urgentPointer : 16;
    uint32_t *options;
    uint8_t *payload;
} __attribute__((packed));

const uint8_t FIN_FLAG = 1;
const uint8_t SYN_FLAG = 2;
const uint8_t ACK_FLAG = 16;
const uint8_t SYN_ACK_FLAG = SYN_FLAG | ACK_FLAG;
const uint8_t FIN_ACK_FLAG = FIN_FLAG | ACK_FLAG;

const size_t BASE_SEGMENT_SIZE = sizeof(Segment) - sizeof(uint32_t *) - sizeof(uint8_t *);

Segment initializeSegment();

/**
 * Generate Segment that contain SYN packet
 */
Segment syn(uint32_t seqNum);

/**
 * Generate Segment that contain ACK packet
 */
Segment ack(uint32_t seqNum, uint32_t ackNum);

/**
 * Generate Segment that contain SYN-ACK packet
 */
Segment synAck(uint32_t seqNum, uint32_t ackNum);

/**
 * Generate Segment that contain FIN packet
 */
Segment fin();

/**
 * Generate Segment that contain FIN-ACK packet
 */
Segment finAck();

/**
 * Calculate checksum of a segment
 */
uint8_t *calculateChecksum(Segment segment);

/**
 * Return a new segment with a calcuated checksum fields
 */
Segment updateChecksum(Segment segment);

/**
 * Check if a TCP Segment has a valid checksum
 */
bool isValidChecksum(Segment segment);

/**
 * Convert the flags to uint8_t
 */
uint8_t flagsToByte(Segment segment);

#endif