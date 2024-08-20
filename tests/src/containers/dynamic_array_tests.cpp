
#include "dynamic_array_tests.h"

#include <containers/dynamic_array.h>
#include <containers/string.h>
#include <core/defines.h>
#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <core/random.h>
#include <renderer/vertex.h>

#include <iostream>

#include "../expect.h"
#include "../utilities/counting_test_object.h"

namespace DynamicArray
{
    u8 DynamicArrayShouldCreateAndDestroy()
    {
        C3D::DynamicArray<int> array(10);

        ExpectEqual(10, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectNotEqual(nullptr, (void*)array.GetData());
        ExpectEqual(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        array.Destroy();

        ExpectEqual(0, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(nullptr, (void*)array.GetData());
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        return true;
    }

    u8 DynamicArrayShouldCreateFromFixedSizeArray()
    {
        C3D::DynamicArray<int> array;

        return true;
    }

    u8 DynamicArrayShouldReserve()
    {
        C3D::DynamicArray<int> array;
        array.Reserve(10);

        ExpectEqual(10, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectNotEqual(nullptr, (void*)array.GetData());
        ExpectEqual(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        array.Destroy();

        ExpectEqual(0, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(nullptr, (void*)array.GetData());
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        return true;
    }

    u8 DynamicArrayShouldReserveWithElementsPresent()
    {
        C3D::DynamicArray<int> array(4);

        array.PushBack(1);
        array.PushBack(2);

        array.Reserve(12);

        ExpectEqual(12, array.Capacity());
        ExpectEqual(2, array.Size());
        ExpectEqual(sizeof(int) * 12, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        ExpectEqual(1, array[0]);
        ExpectEqual(2, array[1]);

        array.Destroy();

        ExpectEqual(0, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(nullptr, (void*)array.GetData());
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        return true;
    }

    u8 DynamicArrayShouldResize()
    {
        C3D::DynamicArray<int> array;
        array.Resize(10);

        ExpectEqual(10, array.Capacity());
        ExpectEqual(10, array.Size());
        ExpectEqual(sizeof(int) * 10, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        for (int i = 0; i < 10; i++)
        {
            ExpectEqual(0, array[i]);
        }

        array.Destroy();

        ExpectEqual(0, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(nullptr, (void*)array.GetData());
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        return true;
    }

    u8 DynamicArrayShouldAllocateLargeBlocks()
    {
        C3D::DynamicArray<C3D::Vertex3D> array(32768);

        ExpectEqual(32768, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(sizeof(C3D::Vertex3D) * 32768, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        constexpr auto element = C3D::Vertex3D{ vec3(0), vec3(0), vec2(1), vec4(1), vec4(4) };
        array.PushBack(element);

        ExpectEqual(element.position, array[0].position);
        ExpectEqual(element.normal, array[0].normal);
        ExpectEqual(element.texture, array[0].texture);
        ExpectEqual(element.color, array[0].color);
        ExpectEqual(element.tangent, array[0].tangent);

        array.Destroy();

        ExpectEqual(0, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(nullptr, (void*)array.GetData());
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        return true;
    }

    u8 DynamicArrayShouldAllocateLargeBlocksAndCopyOverElementsOnResize()
    {
        C3D::DynamicArray<C3D::Vertex3D> array(4);

        ExpectEqual(4, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(sizeof(C3D::Vertex3D) * 4, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        constexpr auto element = C3D::Vertex3D{ vec3(0), vec3(0), vec2(1), vec4(1), vec4(4) };
        array.PushBack(element);

        ExpectEqual(element.position, array[0].position);
        ExpectEqual(element.normal, array[0].normal);
        ExpectEqual(element.texture, array[0].texture);
        ExpectEqual(element.color, array[0].color);
        ExpectEqual(element.tangent, array[0].tangent);

        array.Reserve(32768);

        ExpectEqual(32768, array.Capacity());
        ExpectEqual(1, array.Size());
        ExpectEqual(sizeof(C3D::Vertex3D) * 32768, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::DynamicArray));

        ExpectEqual(element.position, array[0].position);
        ExpectEqual(element.normal, array[0].normal);
        ExpectEqual(element.texture, array[0].texture);
        ExpectEqual(element.color, array[0].color);
        ExpectEqual(element.tangent, array[0].tangent);

        array.Destroy();

        ExpectEqual(0, array.Capacity());
        ExpectEqual(0, array.Size());
        ExpectEqual(nullptr, (void*)array.GetData());
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        return true;
    }

    u8 DynamicArrayShouldReallocate()
    {
        using namespace C3D::Tests;

        {
            C3D::DynamicArray<CountingObject> array;

            array.PushBack(CountingObject());
            array.PushBack(CountingObject());
            array.PushBack(CountingObject());
            array.PushBack(CountingObject());

            ExpectEqual(4, TEST_OBJECT_COUNTER);
            ExpectEqual(4, array.Size());
            ExpectEqual(4, array.Capacity());

            array.PushBack(CountingObject());
            array.PushBack(CountingObject());
            array.PushBack(CountingObject());
            array.PushBack(CountingObject());

            ExpectEqual(8, TEST_OBJECT_COUNTER);
            ExpectEqual(8, array.Size());
        }

        ExpectEqual(0, TEST_OBJECT_COUNTER);

        return true;
    }

    u8 DynamicArrayShouldIterate()
    {
        C3D::DynamicArray<int> array;
        array.PushBack(5);
        array.PushBack(6);
        array.PushBack(2);

        int counter = 0;
        for (auto& elem : array)
        {
            counter++;
        }

        ExpectEqual(array.Size(), counter);
        return true;
    }

    u8 DynamicArrayShouldDestroyWhenLeavingScope()
    {
        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));

        {
            C3D::DynamicArray<C3D::String> array;
            array.PushBack("Test");
            array.PushBack("Test2");
        }

        ExpectEqual(0, Metrics.GetMemoryUsage(C3D::MemoryType::DynamicArray));
        return true;
    }

    struct TestElement
    {
        explicit TestElement(int* c)
        {
            pCounter = c;
            *pCounter += 1;
        }

        TestElement(const TestElement&) = delete;
        TestElement(TestElement&&)      = delete;

        TestElement& operator=(const TestElement& other)
        {
            if (this != &other)
            {
                pCounter = other.pCounter;
            }
            return *this;
        }

        TestElement& operator=(TestElement&&) = delete;

        ~TestElement() { *pCounter -= 1; }

        int* pCounter;
    };

    u8 DynamicArrayShouldCallDestructorsOfElementsWhenDestroyed()
    {
        auto counter = 0;

        C3D::DynamicArray<TestElement> array;
        array.EmplaceBack(&counter);
        array.EmplaceBack(&counter);

        ExpectEqual(2, counter);

        array.Destroy();

        ExpectEqual(0, counter);
        return true;
    }

    u8 DynamicArrayShouldCallDestructorsOfElementsWhenGoingOutOfScope()
    {
        auto counter = 0;

        {
            C3D::DynamicArray<TestElement> array;
            array.EmplaceBack(&counter);
            array.EmplaceBack(&counter);

            ExpectEqual(2, counter);
        }

        ExpectEqual(0, counter);
        return true;
    }

    u8 DynamicArrayShouldFindAndErase()
    {
        C3D::DynamicArray<int> array;

        array.PushBack(5);
        array.PushBack(6);
        array.PushBack(7);
        array.PushBack(8);

        const auto it = std::find(array.begin(), array.end(), 6);
        array.Erase(it);

        ExpectEqual(5, array[0]);
        ExpectEqual(7, array[1]);
        ExpectEqual(8, array[2]);

        return true;
    }

    u8 DynamicArrayEraseLast()
    {
        C3D::DynamicArray<int> array;

        array.PushBack(5);
        array.PushBack(6);
        array.PushBack(7);
        array.PushBack(8);

        const auto it = std::find(array.begin(), array.end(), 8);
        array.Erase(it);

        ExpectEqual(5, array[0]);
        ExpectEqual(6, array[1]);
        ExpectEqual(7, array[2]);
        ExpectEqual(3, array.Size());

        return true;
    }

    u8 DynamicArrayEraseByIndex()
    {
        C3D::DynamicArray<int> array;

        array.PushBack(5);
        array.PushBack(6);
        array.PushBack(7);
        array.PushBack(8);

        array.Erase(2);

        ExpectEqual(5, array[0]);
        ExpectEqual(6, array[1]);
        ExpectEqual(8, array[2]);
        ExpectEqual(3, array.Size());

        return true;
    }

    u8 DynamicArrayEraseByIndexLast()
    {
        C3D::DynamicArray<int> array;

        array.PushBack(1);
        array.PushBack(2);
        array.PushBack(3);
        array.PushBack(4);

        for (auto i = array.SSize() - 1; i >= 0; i--)
        {
            array.Erase(i);
            if (i != 0)
            {
                ExpectEqual(i, array[i - 1]);
                ExpectEqual(i, array.Size());
            }
        }

        return true;
    }

    u8 DynamicArrayShouldInsert()
    {
        C3D::DynamicArray array = { 1, 2, 4, 5 };
        array.Insert(array.begin() + 2, 3);

        for (u64 i = 0; i < array.Size(); i++)
        {
            ExpectEqual(i + 1, array[i]);
        }

        return true;
    }

    u8 DynamicArrayShouldInsertRange()
    {
        C3D::DynamicArray array = { 1, 6 };
        C3D::DynamicArray range = { 2, 3, 4, 5 };

        array.Insert(array.begin() + 1, range.begin(), range.end());

        for (u64 i = 0; i < array.Size(); i++)
        {
            ExpectEqual(i + 1, array[i]);
        }

        return true;
    }

    u8 DynamicArrayShouldPreserveExistingElementsWhenReserveIsCalled()
    {
        C3D::DynamicArray array = { 0, 1, 2, 3 };
        array.Reserve(32);

        for (u64 i = 0; i < array.Size(); i++)
        {
            ExpectEqual(i, array[i]);
        }

        return true;
    }

    u8 DynamicArrayShouldShrinkCorrectly()
    {
        C3D::DynamicArray<int> array;
        // We make sure our capacity is quite high
        array.Reserve(16);
        // Then we add elements (but not enough to fill the array)
        array.PushBack(1);
        array.PushBack(2);
        array.PushBack(3);
        array.PushBack(4);

        ExpectNotEqual(array.Size(), array.Capacity());
        ExpectEqual(4, array.Size());

        array.ShrinkToFit();

        // After shrinking our capacity should match our size
        ExpectEqual(array.Size(), array.Capacity());
        ExpectEqual(4, array.Size());

        return true;
    }

    u8 DynamicArrayShouldClearCorrectly()
    {
        C3D::DynamicArray<C3D::String> array;

        array.EmplaceBack();
        array.EmplaceBack();
        array.EmplaceBack();
        array.EmplaceBack();

        array.Clear();

        ExpectEqual(0, array.Size());
        ExpectEqual(4, array.Capacity());

        return true;
    }

    u8 DynamicArrayShouldNotDoAnythingWhenResizeIsCalledWithASmallerSize()
    {
        C3D::DynamicArray<int> array;
        array.Reserve(20);

        array.PushBack(1);
        array.PushBack(2);
        array.PushBack(3);
        array.PushBack(4);

        array.Resize(5);

        ExpectEqual(20, array.Capacity());
        ExpectEqual(5, array.Size());

        ExpectEqual(1, array[0]);
        ExpectEqual(2, array[1]);
        ExpectEqual(3, array[2]);
        ExpectEqual(4, array[3]);
        ExpectEqual(0, array[4]);

        return true;
    }

    u8 DynamicArrayShouldRemove()
    {
        C3D::DynamicArray<C3D::String> array;
        array.Reserve(10);

        array.PushBack("Test");
        array.PushBack("Test2");
        array.PushBack("Test3");
        array.PushBack("Test4");
        array.PushBack("Test5");

        ExpectEqual(5, array.Size());
        ExpectTrue(array.Remove("Test3"));

        ExpectEqual("Test", array[0]);
        ExpectEqual("Test2", array[1]);
        ExpectEqual("Test4", array[2]);
        ExpectEqual("Test5", array[3]);

        ExpectEqual(4, array.Size());

        return true;
    }

    class CopyConstructorObject
    {
    public:
        CopyConstructorObject()
        {
            m_size          = 100;
            m_ptr           = Memory.Allocate<int>(C3D::MemoryType::Test, m_size);
            auto randomInts = C3D::Random.GenerateMultiple(100, 0, 100);
            for (u32 i = 0; i < 100; i++)
            {
                m_ptr[i] = randomInts[i];
            }
        }

        CopyConstructorObject(const CopyConstructorObject& other) { Copy(other); }
        CopyConstructorObject& operator=(const CopyConstructorObject& other)
        {
            Copy(other);
            return *this;
        }

        CopyConstructorObject(CopyConstructorObject&&)            = delete;
        CopyConstructorObject& operator=(CopyConstructorObject&&) = delete;

        ~CopyConstructorObject()
        {
            if (m_ptr && m_size > 0)
            {
                Memory.Free(m_ptr);
                m_ptr  = nullptr;
                m_size = 0;
            }
        }

        bool Match(const CopyConstructorObject& other)
        {
            for (u32 i = 0; i < 100; i++)
            {
                if (m_ptr[i] != other.m_ptr[i]) return false;
            }
            return true;
        }

        void Copy(const CopyConstructorObject& other)
        {
            if (this != &other)
            {
                if (m_ptr && m_size > 0)
                {
                    Memory.Free(m_ptr);
                    m_ptr  = nullptr;
                    m_size = 0;
                }

                if (other.m_ptr)
                {
                    m_ptr  = Memory.Allocate<int>(C3D::MemoryType::Test, other.m_size);
                    m_size = other.m_size;

                    for (u32 i = 0; i < other.m_size; i++)
                    {
                        m_ptr[i] = other.m_ptr[i];
                    }
                }
            }
        }

    private:
        int* m_ptr = nullptr;
        u64 m_size = 0;
    };

    u8 DynamicArrayShouldReallocWithNonTrivialCopyConstructorObjects()
    {
        CopyConstructorObject obj1;
        CopyConstructorObject obj2;
        CopyConstructorObject obj3;
        CopyConstructorObject obj4;
        CopyConstructorObject obj5;

        C3D::DynamicArray<CopyConstructorObject> array;
        // Push 5 items to ensure we need to do a realloc (default size == 4)
        array.PushBack(obj1);
        array.PushBack(obj2);
        array.PushBack(obj3);
        array.PushBack(obj4);
        array.PushBack(obj5);

        // After the realloc all our objects should still match
        ExpectTrue(obj1.Match(array[0]));
        ExpectTrue(obj2.Match(array[1]));
        ExpectTrue(obj3.Match(array[2]));
        ExpectTrue(obj4.Match(array[3]));
        ExpectTrue(obj5.Match(array[4]));

        return true;
    }

    void RegisterTests(TestManager& manager)
    {
        manager.StartType("DynamicArray");
        REGISTER_TEST(DynamicArrayShouldCreateAndDestroy,
                      "Dynamic array should internally allocate memory and destroy it properly on destroy");
        REGISTER_TEST(DynamicArrayShouldReserve, "Dynamic array should internally allocate enough space after Reserve() call");
        REGISTER_TEST(DynamicArrayShouldReserveWithElementsPresent,
                      "Dynamic array should internally allocate enough space after Reserve() call");
        REGISTER_TEST(DynamicArrayShouldResize, "Dynamic array should Resize() and allocate enough memory with default objects");
        REGISTER_TEST(DynamicArrayShouldAllocateLargeBlocks,
                      "Dynamic array should also work when allocating lots of storage for complex structs");
        REGISTER_TEST(DynamicArrayShouldAllocateLargeBlocksAndCopyOverElementsOnResize,
                      "Dynamic array should also work when allocating lots of storage for complex structs after at "
                      "least 1 element is added");
        REGISTER_TEST(DynamicArrayShouldReallocate,
                      "Dynamic array should reallocate whenever there is not enough space and also cleanup the old memory.");
        REGISTER_TEST(DynamicArrayShouldIterate, "Dynamic array should iterate over all it's elements in a foreach loop");
        REGISTER_TEST(DynamicArrayShouldDestroyWhenLeavingScope,
                      "Dynamic array should be automatically destroyed and cleaned up after leaving scope");
        REGISTER_TEST(DynamicArrayShouldCallDestructorsOfElementsWhenDestroyed,
                      "Dynamic array should automatically call the destructor of every element when it is destroyed");
        REGISTER_TEST(DynamicArrayShouldCallDestructorsOfElementsWhenGoingOutOfScope,
                      "Dynamic array should automatically call the destructor of every element when it goes out of scope");
        REGISTER_TEST(DynamicArrayShouldFindAndErase,
                      "Dynamic array should erase elements based on iterator and move all elements after it to the left");
        REGISTER_TEST(DynamicArrayEraseLast, "Dynamic array should erase last element correctly");
        REGISTER_TEST(DynamicArrayEraseByIndex, "Dynamic array should erase by index");
        REGISTER_TEST(DynamicArrayEraseByIndexLast, "Dynamic array should erase by index for the last element");
        REGISTER_TEST(DynamicArrayShouldInsert, "Dynamic array should insert elements at a random iterator location");
        REGISTER_TEST(DynamicArrayShouldInsertRange, "Dynamic array should insert range of elements at a random iterator location ");
        REGISTER_TEST(DynamicArrayShouldPreserveExistingElementsWhenReserveIsCalled,
                      "If you call reserve on a dynamic array with elements already present they should be preserved");
        REGISTER_TEST(DynamicArrayShouldShrinkCorrectly, "Dynamic array should shrink correctly");
        REGISTER_TEST(DynamicArrayShouldClearCorrectly, "Dynamic array should have size == 0 and capacity == unchanged after a Clear()");
        REGISTER_TEST(DynamicArrayShouldNotDoAnythingWhenResizeIsCalledWithASmallerSize,
                      "Dynamic array should not do anything when resize is called with a smaller size then current capacity");
        REGISTER_TEST(DynamicArrayShouldRemove, "Dynamic array should be able to remove element by value");
        REGISTER_TEST(DynamicArrayShouldReallocWithNonTrivialCopyConstructorObjects,
                      "Dynamic array should be able to realloc also with non-trivial copy constructors");
    }

}  // namespace DynamicArray
