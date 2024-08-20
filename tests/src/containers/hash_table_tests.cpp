
#include "hash_table_tests.h"

#include <containers/hash_table.h>
#include <core/defines.h>

#include "../expect.h"
#include "core/metrics/metrics.h"

u8 HashTableShouldCreateAndDestroy()
{
    C3D::HashTable<int> hashtable;
    ExpectTrue(hashtable.Create(128));

    ExpectEqual(128 * sizeof(int), C3D::HashTable<int>::GetMemoryRequirement(128));
    ExpectEqual(128 * sizeof(int), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::HashTable));

    hashtable.Destroy();

    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashTable)) return true;
}

u8 HashTableShouldSetAndGetForValueTypes()
{
    C3D::HashTable<int> hashtable;
    ExpectTrue(hashtable.Create(128));

    int testValue = 5;

    ExpectTrue(hashtable.Set("test", testValue)) ExpectEqual(testValue, hashtable.Get("test"));

    hashtable.Destroy();

    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashTable)) return true;
}

u8 HashTableShouldSetAndGetForPointerTypes()
{
    C3D::HashTable<int*> hashtable;
    ExpectTrue(hashtable.Create(128));

    int testValue   = 5;
    int* pTestValue = &testValue;

    ExpectTrue(hashtable.Set("test", pTestValue)) ExpectEqual(testValue, *hashtable.Get("test"));

    hashtable.Destroy();

    ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashTable)) return true;
}

void HashTable::RegisterTests(TestManager& manager)
{
    manager.StartType("HashTable");
    REGISTER_TEST(HashTableShouldCreateAndDestroy, "HashTable internal allocation and free should happen properly.");
    REGISTER_TEST(HashTableShouldSetAndGetForValueTypes, "HashTable Set and Get should work for value types.");
    REGISTER_TEST(HashTableShouldSetAndGetForPointerTypes, "HashTable Set and Get should work for pointer types.");
}