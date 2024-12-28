#ifndef UTILS_H
#define UTILS_H

#include "segment.hpp"
// #include <stdlib.h>
// #include <iostream>
// #include <string>
// #include <cstring>
// #include <fstream>
// #include <vector>

uint8_t *serializeSegment(Segment *toBeSent, size_t optionsLen, size_t payloadLen);
void deserializeToSegment(Segment *segment, uint8_t *buffer, size_t nBytes);
void printSegment(const Segment &seg, size_t payloadLen);

#endif
