#ifndef utils_h
#define utils_h

#include "segment.hpp"

uint8_t *serializeSegment(Segment *toBeSent, size_t options_len, size_t payload_len);
void deserializeToSegment(Segment *segment, uint8_t *buffer, size_t nBytes);
void printSegment(const Segment &seg, size_t payload_len);

#endif
