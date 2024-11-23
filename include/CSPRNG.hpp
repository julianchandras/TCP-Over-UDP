#ifndef CSPRNG_H
#define CSPRNG_H

#include <fstream>

class CSPRNG
{
public:
    CSPRNG();
    ~CSPRNG();
    uint32_t getRandomUInt32();
    uint32_t getRandomInRange(uint32_t min, uint32_t max);

private:
    std::ifstream urandom;
};

#endif