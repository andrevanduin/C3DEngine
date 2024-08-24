
#include <containers/string.h>
#include <core/logger.h>
#include <math/c3d_math.h>

class ExpectException : public std::exception
{
public:
    ExpectException(const char* msg) : exception(msg) {}
};

/**
 * @brief Expects expected to be equal to actual.
 */
#define ExpectEqual(expected, actual)                                                                                                 \
    {                                                                                                                                 \
        auto _exp = (expected);                                                                                                       \
        auto _act = (actual);                                                                                                         \
        if (_exp != _act)                                                                                                             \
        {                                                                                                                             \
            auto msg = C3D::String::FromFormat("Expected {} == {}, but expected was: {} and actual was: {}. File: {}:{}.", #expected, \
                                               #actual, _exp, _act, __FILE__, __LINE__);                                              \
            throw ExpectException(msg.Data());                                                                                        \
        }                                                                                                                             \
    }

/**
 * @brief Expects expected to NOT be equal to actual.
 */
#define ExpectNotEqual(expected, actual)                                                                                              \
    {                                                                                                                                 \
        auto _exp = (expected);                                                                                                       \
        auto _act = (actual);                                                                                                         \
        if (_exp == _act)                                                                                                             \
        {                                                                                                                             \
            auto msg = C3D::String::FromFormat("Expected {} != {}, but expected was: {} and actual was: {}. File: {}:{}.", #expected, \
                                               #actual, _exp, _act, __FILE__, __LINE__);                                              \
            throw(msg.Data());                                                                                                        \
        }                                                                                                                             \
    }

/**
 * @brief Expects expected to be actual given a tolerance of K_FLOAT_EPSILON.
 */
#define ExpectFloatEqual(expected, actual)                                                                                            \
    {                                                                                                                                 \
        auto e = (expected);                                                                                                          \
        auto a = (actual);                                                                                                            \
        if (kabs(_exp - _act) > 0.001f)                                                                                               \
        {                                                                                                                             \
            auto msg = C3D::String::FromFormat("Expected {} == {}, but expected was: {} and actual was: {}. File: {}:{}.", #expected, \
                                               #actual, _exp, _act, __FILE__, __LINE__);                                              \
            throw(msg.Data());                                                                                                        \
        }                                                                                                                             \
    }

/**
 * @brief Expects actual to be true.
 */
#define ExpectTrue(actual)                                                                                                              \
    {                                                                                                                                   \
        auto _act = (actual);                                                                                                           \
        if (!_act)                                                                                                                      \
        {                                                                                                                               \
            auto msg =                                                                                                                  \
                C3D::String::FromFormat("Expected {} to be true but it evaluated to false. File: {}:{}.", #actual, __FILE__, __LINE__); \
            throw(msg.Data());                                                                                                          \
        }                                                                                                                               \
    }

/**
 * @brief Expects actual to be false.
 */
#define ExpectFalse(actual)                                                                                                             \
    {                                                                                                                                   \
        auto _act = (actual);                                                                                                           \
        if (_act)                                                                                                                       \
        {                                                                                                                               \
            auto msg =                                                                                                                  \
                C3D::String::FromFormat("Expected {} to be false but it evaluated to true. File: {}:{}.", #actual, __FILE__, __LINE__); \
            throw(msg.Data());                                                                                                          \
        }                                                                                                                               \
    }

#define AssertFail(error)                                                    \
    {                                                                        \
        auto msg = C3D::String::FromFormat("Asserted failure: {}", (error)); \
        throw(msg.Data());                                                   \
    }

#define ExpectThrow(errorType, func)                                                               \
    {                                                                                              \
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
        }                                                                                          \
    }