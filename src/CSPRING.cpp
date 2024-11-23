#include "../include/CSPRNG.hpp"

class CSPRNG
{
public:
    CSPRNG::CSPRNG()
    {
        urandom.open("/dev/urandom", std::ios::in | std::ios::binary);
        if (!urandom.is_open())
        {
            throw std::runtime_error("Failed to open /dev/urandom");
        }
    }

    CSPRNG::~CSPRNG()
    {
        if (urandom.is_open())
        {
            urandom.close();
        }
    }

    uint32_t CSPRNG::getRandomUInt32()
    {
        uint32_t randomValue;
        urandom.read(reinterpret_cast<char *>(&randomValue), sizeof(randomValue));
        if (!urandom)
        {
            throw std::runtime_error("Failed to read from /dev/urandom");
        }
        return randomValue;
    }

    uint32_t CSPRNG::getRandomInRange(uint32_t min, uint32_t max)
    {
        if (min > max)
        {
            throw std::invalid_argument("Invalid range: min > max");
        }
        uint32_t range = max - min + 1;
        return min + (getRandomUInt32() % range);
    }

private:
    std::ifstream urandom;
};