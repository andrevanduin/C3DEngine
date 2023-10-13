
#include "hash_map_tests.h"

#include <containers/hash_map.h>
#include <core/defines.h>

#include "../expect.h"
#include "containers/string.h"
#include "core/metrics/metrics.h"

u8 HashMapShouldCreateAndDestroy()
{
    C3D::HashMap<C3D::String, u32> hashMap;
    hashMap.Create(128);

    ExpectShouldBe(128 * sizeof(u32) + 128 * sizeof(u32), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::HashMap));

    hashMap.Destroy();

    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashMap));
    return true;
}

u8 HashMapShouldSetAndGet()
{
    C3D::HashMap<C3D::String, u32> hashMap;
    hashMap.Create(128);

    hashMap.Set("Test", 5);

    ExpectShouldBe(5, hashMap.Get("Test"));

    return true;
}

u8 HashMapGetShouldBeEditable()
{
    C3D::HashMap<C3D::String, u32> hashMap;
    hashMap.Create(128);

    hashMap.Set("Test", 5);
    ExpectShouldBe(5, hashMap.Get("Test"));

    auto& test = hashMap.Get("Test");
    test       = 12;

    ExpectShouldBe(12, hashMap.Get("Test"));
    return true;
}

u8 HashMapHasShouldWork()
{
    C3D::HashMap<const char*, C3D::String> hashMap;
    hashMap.Create(128);

    hashMap.Set("Test", "Other Test");
    ExpectToBeTrue(hashMap.Has("Test"));
    ExpectToBeFalse(hashMap.Has("Test1234"));

    return true;
}

u8 HashMapShouldIterate()
{
    C3D::HashMap<C3D::String, u32> hashMap;
    hashMap.Create(128);

    hashMap.Set("test", 1);
    hashMap.Set("twelve", 2);
    hashMap.Set("other", 3);

    std::vector<u32> values = { 1, 2, 3 };

    for (const auto item : hashMap)
    {
        const auto it = std::find(values.begin(), values.end(), item);
        // All values we find need to exist
        ExpectToBeTrue(it != values.end());
        // Set the value to 0 so we know we have seen it
        *it = 0;
    }

    // We expect all the values to be found (so equal to 0)
    for (const auto item : values)
    {
        ExpectShouldBe(0, item);
    }

    return true;
}

void HashMap::RegisterTests(TestManager& manager)
{
    manager.StartType("HashMap");
    manager.Register(HashMapShouldCreateAndDestroy, "HashMap should create and destroy correctly.");
    manager.Register(HashMapShouldSetAndGet,
                     "You should be able to set an entry in the HashMap by key and get it with the same key.");
    manager.Register(HashMapGetShouldBeEditable, "You should be able to get and entry by key and edit it.");
    manager.Register(HashMapHasShouldWork,
                     "HashMap Has() function should return true if key already exists and false otherwise.");
    manager.Register(HashMapShouldIterate, "You should be able to iterate over all existing elements.");
}