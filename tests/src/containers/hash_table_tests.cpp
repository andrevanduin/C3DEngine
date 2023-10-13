
#include "hash_table_tests.h"
#include "../expect.h"

#include <containers/hash_table.h>
#include <core/defines.h>

#include "core/metrics/metrics.h"

u8 HashTableShouldCreateAndDestroy()
{
	C3D::HashTable<int> hashtable;
	ExpectToBeTrue(hashtable.Create(128));

	ExpectShouldBe(128 * sizeof(int), C3D::HashTable<int>::GetMemoryRequirement(128))
	ExpectShouldBe(128 * sizeof(int), Metrics.GetRequestedMemoryUsage(C3D::MemoryType::HashTable))

	hashtable.Destroy();

	ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashTable))
	return true;
}

u8 HashTableShouldSetAndGetForValueTypes()
{
	C3D::HashTable<int> hashtable;
	ExpectToBeTrue(hashtable.Create(128))

	int testValue = 5;

	ExpectToBeTrue(hashtable.Set("test", testValue))
	ExpectShouldBe(testValue, hashtable.Get("test"))

	hashtable.Destroy();

	ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashTable))
	return true;
}

u8 HashTableShouldSetAndGetForPointerTypes()
{
	C3D::HashTable<int*> hashtable;
	ExpectToBeTrue(hashtable.Create(128))

	int testValue = 5;
	int* pTestValue = &testValue;

	ExpectToBeTrue(hashtable.Set("test", pTestValue))
	ExpectShouldBe(testValue, *hashtable.Get("test"))

	hashtable.Destroy();

	ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::HashTable))
	return true;
}

void HashTable::RegisterTests(TestManager& manager)
{
	manager.StartType("HashTable");
	manager.Register(HashTableShouldCreateAndDestroy, "HashTable internal allocation and free should happen properly.");
	manager.Register(HashTableShouldSetAndGetForValueTypes, "HashTable Set and Get should work for value types.");
	manager.Register(HashTableShouldSetAndGetForPointerTypes, "HashTable Set and Get should work for pointer types.");
}