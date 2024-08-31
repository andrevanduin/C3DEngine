
#include "hash_map_tests.h"

#include <containers/hash_map.h>
#include <defines.h>
#include <metrics/metrics.h>
#include <random/random.h>
#include <string/string.h>

#include "../expect.h"
#include "../utilities/non_trivial_destructor_object.h"

namespace HashMap
{
    TEST(HashMapShouldCreateAndDestroy)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        ExpectEqual(C3D::HASH_MAP_DEFAULT_CAPACITY, hashMap.Capacity());
        ExpectEqual(C3D::HASH_MAP_DEFAULT_CAPACITY * sizeof(C3D::HashMap<C3D::String, u32>::Node),
                    Metrics.GetRequestedMemoryUsage(C3D::MemoryType::HashMap));
        ExpectEqual(C3D::HASH_MAP_DEFAULT_LOAD_FACTOR, hashMap.LoadFactor());

        hashMap.Destroy();

        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashMap));
    }

    TEST(HashMapShouldSetAndGet)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        hashMap.Set("Test", 5);

        ExpectEqual(5, hashMap.Get("Test"));
    }

    TEST(HashMapGetShouldBeEditable)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        hashMap.Set("Test", 5);
        ExpectEqual(5, hashMap.Get("Test"));

        auto& test = hashMap.Get("Test");
        test       = 12;

        ExpectEqual(12, hashMap.Get("Test"));
    }

    TEST(HashMapHasShouldWork)
    {
        const char* test = "Test";

        C3D::HashMap<const char*, C3D::String> hashMap;
        hashMap.Create();

        hashMap.Set(test, "Other Test");
        ExpectTrue(hashMap.Has(test));
        ExpectFalse(hashMap.Has("Test1234"));
    }

    TEST(HashMapShouldIterate)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        hashMap.Set("test", 1);
        hashMap.Set("twelve", 2);
        hashMap.Set("other", 3);

        std::vector<u32> values = { 1, 2, 3 };

        for (const auto item : hashMap)
        {
            const auto it = std::find(values.begin(), values.end(), item);
            // All values we find need to exist
            ExpectTrue(it != values.end());
            // Set the value to 0 so we know we have seen it
            *it = 0;
        }

        // We expect all the values to be found (so equal to 0)
        for (const auto item : values)
        {
            ExpectEqual(0, item);
        }
    }

    TEST(HashMapShouldOverrideForDuplicateKeys)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        hashMap.Insert("Test", 1);
        hashMap.Insert("Test", 2);
        hashMap.Insert("Test", 3);
        hashMap.Insert("Test", 4);
        hashMap.Insert("Test", 5);
        hashMap.Insert("Test", 6);
        hashMap.Insert("Test", 7);
        hashMap.Insert("Test", 8);
        hashMap.Insert("Test", 9);
        hashMap.Insert("Test", 10);

        ExpectEqual(10, hashMap.Get("Test"));
        ExpectEqual(1, hashMap.Count());
    }

    TEST(HashMapShouldWorkWhenGettingCloseToLoadFactor)
    {
        // Default LoadFactor == 0.75 and capacity == 32 so at 32 * 0.75 = 24 we have reached our LoadFactor
        // We insert 23 items to test verify our HashMap works as expected without it growing (which happens at count >= LoadFactor *
        // capacity)
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        // Keep in mind that this is very much a worst case scenario where all keys are very similar
        for (u32 i = 1; i < 25; i++)
        {
            hashMap.Insert(C3D::String::FromFormat("Test{}", i), i);
        }

        ExpectEqual(24, hashMap.Count());
        ExpectEqual(C3D::HASH_MAP_DEFAULT_CAPACITY, hashMap.Capacity());

        for (u32 i = 1; i < 25; i++)
        {
            auto str = C3D::String::FromFormat("Test{}", i);
            ExpectTrue(hashMap.Contains(str));
            ExpectEqual(i, hashMap.Get(str));
        }
    }

    TEST(HashMapDeleteShouldWorkAsExpected)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        hashMap.Set("Test1", 5);
        hashMap.Set("Bla", 15);
        hashMap.Set("Rens", 3);
        hashMap.Set("Feest", 42);

        ExpectEqual(4, hashMap.Count());
        ExpectEqual(5, hashMap.Get("Test1"));
        ExpectEqual(15, hashMap.Get("Bla"));
        ExpectEqual(3, hashMap.Get("Rens"));
        ExpectEqual(42, hashMap.Get("Feest"));

        hashMap.Delete("Bla");

        ExpectEqual(5, hashMap.Get("Test1"));
        ExpectEqual(3, hashMap.Get("Rens"));
        ExpectEqual(42, hashMap.Get("Feest"));
        ExpectFalse(hashMap.Contains("Bla"));
    }

    TEST(HashMapShouldGrowWhenLoadFactorIsReached)
    {
        // Purposfully pick a small load factor so we don't have to add a lot of items before we grow
        C3D::HashMap<C3D::String, u32, std::hash<C3D::String>, 0.1> hashMap;
        hashMap.Create();

        hashMap.Insert("klaas", 208);
        hashMap.Insert("wendy", 17);
        hashMap.Insert("pieter", 84);
        hashMap.Insert("rens", 22);
        hashMap.Insert("bla", 52);

        // We expect our hashmap to have grown
        ExpectEqual(C3D::HASH_MAP_DEFAULT_CAPACITY * 2, hashMap.Capacity());

        // We also expect all our keys to still be found properly
        ExpectEqual(208, hashMap.Get("klaas"));
        ExpectEqual(17, hashMap.Get("wendy"));
        ExpectEqual(84, hashMap.Get("pieter"));
        ExpectEqual(22, hashMap.Get("rens"));
        ExpectEqual(52, hashMap.Get("bla"));
    }

    TEST(HashMapShouldNotLeakMemory)
    {
        using namespace C3D::Tests;

        {
            // Check if the HashMap does not leak after inserting some items
            C3D::HashMap<C3D::String, NonTrivialDestructorObject> hashMap;
            hashMap.Create();

            // Ensure that the HashMap must grow in size at least once
            for (u32 i = 1; i <= 4; ++i)
            {
                auto obj = NonTrivialDestructorObject();
                hashMap.Insert(C3D::String::FromFormat("test{}", i), obj);
            }
        }

        // We expect all HashMap memory to be cleaned up
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashMap));
        // We also expect all internal Key (string) memory to be cleaned up
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::String));
        // and also our Value memory to be cleaned up
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::Test));

        {
            // Check if the HashMap does not leak after it has grown at least once
            C3D::HashMap<C3D::String, NonTrivialDestructorObject> hashMap;
            hashMap.Create();

            // Ensure that the HashMap must grow in size at least once
            for (u32 i = 1; i <= 32; ++i)
            {
                auto obj = NonTrivialDestructorObject();
                hashMap.Insert(C3D::String::FromFormat("test{}", i), obj);
            }
        }

        // We expect all HashMap memory to be cleaned up
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashMap));
        // We also expect all internal Key (string) memory to be cleaned up
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::String));
        // and also our Value memory to be cleaned up

        C3D::Logger::Info("Test Allocs left: {}", Metrics.GetAllocCount(C3D::MemoryType::Test));
        C3D::Logger::Info("Test Memory left: {}", Metrics.GetMemoryUsage(C3D::MemoryType::Test));

        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::Test));
    }

    struct TestObject
    {
        C3D::String key;
        u32 value;
        bool deleted = false;
    };

    TEST(HashMapStressTest)
    {
        C3D::HashMap<C3D::String, u32> hashMap;
        hashMap.Create();

        std::vector<TestObject> objects;

        // Generate 128 random <Key, Value> pairs
        for (u32 i = 0; i < 128; ++i)
        {
            TestObject obj;
            obj.key   = C3D::Random.GenerateString(4, 10);
            obj.value = C3D::Random.Generate(0, 100);
            objects.push_back(obj);
            hashMap.Insert(obj.key, obj.value);
        }

        // With 100 items our HashMap should have grown 3 times
        ExpectEqual(C3D::HASH_MAP_DEFAULT_CAPACITY * 2 * 2 * 2, hashMap.Capacity());

        // Randomly delete +-32 of our entries
        for (u32 i = 0; i < 32; ++i)
        {
            u32 index = C3D::Random.Generate(0, 127);
            auto& obj = objects[index];

            if (!obj.deleted)
            {
                // Only delete if we haven't already done so
                hashMap.Delete(obj.key);
                obj.deleted = true;
            }
        }

        for (auto& obj : objects)
        {
            if (obj.deleted)
            {
                // If the key was removed the HashMap should no longer contain it
                ExpectFalse(hashMap.Contains(obj.key));
            }
            else
            {
                // Otherwise our HashMap should still contain it
                ExpectEqual(obj.value, hashMap.Get(obj.key));
            }
        }
    }

    void RegisterTests(TestManager& manager)
    {
        manager.StartType("HashMap");

        REGISTER_TEST(HashMapShouldCreateAndDestroy, "HashMap should create and destroy correctly.");
        REGISTER_TEST(HashMapShouldSetAndGet, "You should be able to set an entry in the HashMap by key and get it with the same key.");
        REGISTER_TEST(HashMapGetShouldBeEditable, "You should be able to get and entry by key and edit it.");
        REGISTER_TEST(HashMapHasShouldWork, "HashMap Has() function should return true if key already exists and false otherwise.");
        REGISTER_TEST(HashMapShouldIterate, "You should be able to iterate over all existing elements.");
        REGISTER_TEST(HashMapShouldOverrideForDuplicateKeys,
                      "If you insert duplicate keys into the HashMap it should override the existing key instead of adding a new one.");
        REGISTER_TEST(
            HashMapShouldWorkWhenGettingCloseToLoadFactor,
            "When the HashMaps number of items is >= Capacity * LoadFactor it grows. But right before this moment the HashMap should "
            "still function as expected");
        REGISTER_TEST(HashMapDeleteShouldWorkAsExpected, "After calling Delete on an item it should no longer exist in the HashMap.");
        REGISTER_TEST(HashMapShouldGrowWhenLoadFactorIsReached, "A HashMap should grow when it reaches the load factor.");
        REGISTER_TEST(HashMapShouldNotLeakMemory, "The HashMap should not leak memory");
        REGISTER_TEST(HashMapStressTest, "The HashMap should perform as expected with lot's of insertions and deletions.");
    }

}  // namespace HashMap