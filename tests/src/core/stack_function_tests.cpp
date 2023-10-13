
#include "stack_function_tests.h"

#include <containers/string.h>
#include <core/function/function.h>

#include "../expect.h"

static int StaticFunc() { return 5; }

const auto LAMBDA = []() { return 6; };

class TestClass
{
public:
    int MemberFunc() { return 7; }
};

class OperatorClass
{
public:
    int operator()() { return 8; }
};

u8 CreateStaticFunc()
{
    C3D::StackFunction<int(), 16> func(StaticFunc);

    ExpectShouldBe(5, func());

    return true;
}

u8 CreateLambda()
{
    C3D::StackFunction<int(), 16> func(LAMBDA);

    ExpectShouldBe(6, func());

    return true;
}

u8 CreateMemberFunc()
{
    TestClass instance;

    C3D::StackFunction<int(), 16> func([&]() { return instance.MemberFunc(); });

    ExpectShouldBe(7, func());

    return true;
}

u8 CreateClassWithOperator()
{
    OperatorClass instance;

    C3D::StackFunction<int(), 16> func(instance);

    ExpectShouldBe(8, func());

    return true;
}

u8 CopyConstructorShouldWork()
{
    {
        // Static
        C3D::StackFunction<int(), 16> func(StaticFunc);
        // Use copy constructor to copy our func
        C3D::StackFunction<int(), 16> otherFunc(func);

        ExpectShouldBe(5, func());
        ExpectShouldBe(5, otherFunc());
    }

    {
        // Lambda
        C3D::StackFunction<int(), 16> func(LAMBDA);
        // Use copy constructor to copy our func
        C3D::StackFunction<int(), 16> otherFunc(func);

        ExpectShouldBe(6, func());
        ExpectShouldBe(6, otherFunc());
    }

    {
        // Member function
        TestClass instance;

        C3D::StackFunction<int(), 16> func([&]() { return instance.MemberFunc(); });
        // Use copy constructor to copy our func
        C3D::StackFunction<int(), 16> otherFunc(func);

        ExpectShouldBe(7, func());
        ExpectShouldBe(7, otherFunc());
    }

    {
        // Class with operator()
        OperatorClass instance;

        C3D::StackFunction<int(), 16> func(instance);
        // Use copy constructor to copy our func
        C3D::StackFunction<int(), 16> otherFunc(func);

        ExpectShouldBe(8, func());
        ExpectShouldBe(8, otherFunc());
    }

    return true;
}

u8 CopyAssignmentOperatorShouldWork()
{
    {
        // Static
        C3D::StackFunction<int(), 16> func(StaticFunc);
        // Use copy assigment operator
        C3D::StackFunction<int(), 16> otherFunc = func;

        ExpectShouldBe(5, func());
        ExpectShouldBe(5, otherFunc());
    }

    {
        // Lambda
        C3D::StackFunction<int(), 16> func(LAMBDA);
        // Use copy assigment operator
        C3D::StackFunction<int(), 16> otherFunc = func;

        ExpectShouldBe(6, func());
        ExpectShouldBe(6, otherFunc());
    }

    {
        // Member function
        TestClass instance;

        C3D::StackFunction<int(), 16> func([&]() { return instance.MemberFunc(); });
        // Use copy assigment operator
        C3D::StackFunction<int(), 16> otherFunc = func;

        ExpectShouldBe(7, func());
        ExpectShouldBe(7, otherFunc());
    }

    {
        // Class with operator()
        OperatorClass instance;

        C3D::StackFunction<int(), 16> func(instance);
        // Use copy assigment operator
        C3D::StackFunction<int(), 16> otherFunc = func;

        ExpectShouldBe(8, func());
        ExpectShouldBe(8, otherFunc());
    }

    return true;
}

u8 MoveConstructorShouldWork()
{
    {
        // Static
        C3D::StackFunction<int(), 16> func(StaticFunc);
        // Move func into otherFunc
        C3D::StackFunction<int(), 16> otherFunc(std::move(func));

        ExpectShouldBe(5, otherFunc());
    }

    {
        // Lambda
        C3D::StackFunction<int(), 16> func(LAMBDA);
        // Move func into otherFunc
        C3D::StackFunction<int(), 16> otherFunc(std::move(func));

        ExpectShouldBe(6, otherFunc());
    }

    {
        // Member function
        TestClass instance;

        C3D::StackFunction<int(), 16> func([&]() { return instance.MemberFunc(); });
        // Move func into otherFunc
        C3D::StackFunction<int(), 16> otherFunc(std::move(func));

        ExpectShouldBe(7, otherFunc());
    }

    {
        // Class with operator()
        OperatorClass instance;

        C3D::StackFunction<int(), 16> func(instance);
        // Move func into otherFunc
        C3D::StackFunction<int(), 16> otherFunc(std::move(func));

        ExpectShouldBe(8, otherFunc());
    }

    return true;
}

u8 MoveAssignmentOperatorShouldWork()
{
    {
        // Static
        C3D::StackFunction<int(), 16> func(StaticFunc);
        // Use move assignment operartor to assign func to otherFunc
        C3D::StackFunction<int(), 16> otherFunc = std::move(func);

        ExpectShouldBe(5, otherFunc());
    }

    {
        // Lambda
        C3D::StackFunction<int(), 16> func(LAMBDA);
        // Use move assignment operartor to assign func to otherFunc
        C3D::StackFunction<int(), 16> otherFunc = std::move(func);

        ExpectShouldBe(6, otherFunc());
    }

    {
        // Member function
        TestClass instance;

        C3D::StackFunction<int(), 16> func([&]() { return instance.MemberFunc(); });
        // Use move assignment operartor to assign func to otherFunc
        C3D::StackFunction<int(), 16> otherFunc = std::move(func);

        ExpectShouldBe(7, otherFunc());
    }

    {
        // Class with operator()
        OperatorClass instance;

        C3D::StackFunction<int(), 16> func(instance);
        // Use move assignment operartor to assign func to otherFunc
        C3D::StackFunction<int(), 16> otherFunc = std::move(func);

        ExpectShouldBe(8, otherFunc());
    }

    return true;
}

static bool NoArgFunc() { return true; }

static int OneArgFunc(int a) { return a; }

static int AddFunc(int a, int b) { return a + b; }

static int OneConstRefArgFunc(const int& a) { return a - 1; }

static void OneRefArgFunc(int& a) { a += 10; }

static bool CombinationFunc(int a, const int& b, float& c, const C3D::String& d)
{
    c = a + b;
    C3D::Logger::Info("const String& d = {}", d);
    return c >= 4.0f;
}

u8 CallShouldWork()
{
    {
        // No args
        C3D::StackFunction<bool(), 16> func(NoArgFunc);
        ExpectToBeTrue(func());
    }

    {
        // One arg
        C3D::StackFunction<int(int a), 16> func(OneArgFunc);
        ExpectShouldBe(5, func(5));
    }

    {
        // Multiple args
        C3D::StackFunction<int(int a, int b), 16> func(AddFunc);
        ExpectShouldBe(12, func(2, 10));
    }

    {
        // const reference arg
        C3D::StackFunction<int(const int& a), 16> func(OneConstRefArgFunc);

        int a = 5;
        ExpectShouldBe(4, func(a));
    }

    {
        // non-const reference arg
        C3D::StackFunction<void(int& a), 16> func(OneRefArgFunc);

        int a = 5;
        func(a);
        ExpectShouldBe(15, a);
    }

    {
        // Combination of args
        C3D::StackFunction<bool(int a, const int& b, float& c, const C3D::String& d), 16> func(CombinationFunc);

        int b   = 3;
        float c = 0.0f;

        ExpectToBeTrue(func(2, 3, c, "Test with combination of args"));
    }

    return true;
}

void StackFunction::RegisterTests(TestManager& manager)
{
    manager.StartType("StackFunction");
    manager.Register(CreateStaticFunc, "StackFunction should create properly with static function as argument");
    manager.Register(CreateLambda, "StackFunction should create properly with a lambda function as argument");
    manager.Register(CreateMemberFunc, "StackFunction should create properly with member function as argument");
    manager.Register(CreateClassWithOperator,
                     "StackFunction should create properly with a instance of a class that has operator() as argument");
    manager.Register(CopyConstructorShouldWork, "StackFunction copy constructor should work");
    manager.Register(CopyAssignmentOperatorShouldWork, "StackFunction copy assignment operator should work");
    manager.Register(MoveConstructorShouldWork, "StackFunction move constructor should work");
    manager.Register(MoveAssignmentOperatorShouldWork, "StackFunction move assignment operator should work");
    manager.Register(CallShouldWork, "Stackfunction calling should work with multitude of arguments");
}