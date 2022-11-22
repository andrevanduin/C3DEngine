
#include "uuid.h"

namespace C3D
{
    static constexpr auto CHARS = "0123456789abcdef";

	void UUIDS::Seed(u64 seed)
	{
	}

	UUID UUIDS::Generate()
	{
        static std::random_device dev;
        static std::mt19937 rng(dev());
        std::uniform_int_distribution dist(0, 15);

        UUID uuid = {};
        for (u32 i = 0; i < 37; i++)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                uuid.value[i] = '-';
            }
            else
            {
                uuid.value[i] = CHARS[dist(rng)];
            }
        }

        return uuid;
	}
}
