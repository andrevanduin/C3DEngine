
#include <containers/string.h>
#include <core/logger.h>
#include <math/c3d_math.h>

/**
 * @brief Expects expected to be equal to actual.
 */
#define ExpectEqual(expected, actual)                                                                           \
    if ((actual) != (expected))                                                                                 \
    {                                                                                                           \
        C3D::Logger::Error("--> Expected {}, but got: {}. File: {}:{}.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                           \
    }

/**
 * @brief Expects expected to NOT be equal to actual.
 */
#define ExpectNotEqual(expected, actual)                                                                                     \
    if ((actual) == (expected))                                                                                              \
    {                                                                                                                        \
        C3D::Logger::Error("--> Expected {} != {}, but they are equal. File: {}:{}.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                                        \
    }

/**
 * @brief Expects expected to be actual given a tolerance of K_FLOAT_EPSILON.
 */
#define ExpectFloatEqual(expected, actual)                                                                      \
    if (kabs((expected) - (actual)) > 0.001f)                                                                   \
    {                                                                                                           \
        C3D::Logger::Error("--> Expected {}, but got: {}. File: {}:{}.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                           \
    }

/**
 * @brief Expects actual to be true.
 */
#define ExpectTrue(actual)                                                                         \
    if ((actual) != true)                                                                          \
    {                                                                                              \
        C3D::Logger::Error("--> Expected true, but got: false. File: {}:{}.", __FILE__, __LINE__); \
        return false;                                                                              \
    }

/**
 * @brief Expects actual to be false.
 */
#define ExpectFalse(actual)                                                                        \
    if ((actual) != false)                                                                         \
    {                                                                                              \
        C3D::Logger::Error("--> Expected false, but got: true. File: {}:{}.", __FILE__, __LINE__); \
        return false;                                                                              \
    }

#define AssertFail(error)                                \
    C3D::Logger::Error("Asserted failure: {}", (error)); \
    return false;

#define ExpectThrow(errorType, func)                                                           \
    try                                                                                        \
    {                                                                                          \
        func();                                                                                \
        AssertFail("Function should have thrown an exception")                                 \
    }                                                                                          \
    catch (const errorType& ex)                                                                \
    {}                                                                                         \
    catch (...)                                                                                \
    {                                                                                          \
        AssertFail("Function should have thrown (errorType) but threw something else instead") \
    }