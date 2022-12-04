
#include <core/logger.h>
#include <math/c3d_math.h>

/**
 * @brief Expects expected to be equal to actual.
 */
#define ExpectShouldBe(expected, actual)                                                                            \
    if ((actual) != (expected)) {                                                                                   \
        C3D::Logger::Error("--> Expected {}, but got: {}. File: {}:{}.", expected, actual, __FILE__, __LINE__);     \
        return false;                                                                                               \
    }

 /**
  * @brief Expects expected to NOT be equal to actual.
  */
#define ExpectShouldNotBe(expected, actual)                                                                                     \
    if ((actual) == (expected)) {                                                                                               \
        C3D::Logger::Error("--> Expected {} != {}, but they are equal. File: {}:{}.", expected, actual, __FILE__, __LINE__);    \
        return false;                                                                                                           \
    }

  /**
   * @brief Expects expected to be actual given a tolerance of K_FLOAT_EPSILON.
   */
#define ExpectFloatToBe(expected, actual)                                                                           \
    if (kabs((expected) - (actual)) > 0.001f) {                                                                     \
        C3D::Logger::Error("--> Expected {}, but got: {}. File: {}:{}.", expected, actual, __FILE__, __LINE__);     \
        return false;                                                                                               \
    }

   /**
    * @brief Expects actual to be true.
    */
#define ExpectToBeTrue(actual)                                                                              \
    if ((actual) != true) {                                                                                 \
        C3D::Logger::Error("--> Expected true, but got: false. File: {}:{}.", __FILE__, __LINE__);          \
        return false;                                                                                       \
    }

    /**
     * @brief Expects actual to be false.
     */
#define ExpectToBeFalse(actual)                                                                             \
    if ((actual) != false) {                                                                                \
        C3D::Logger::Error("--> Expected false, but got: true. File: {}:{}.", __FILE__, __LINE__);          \
        return false;                                                                                       \
    }

#define AssertFail(error) C3D::Logger::Error("Asserted failure: {}", (error)); return false;

#define ExpectToThrow(errorMsg, func)                               \
	try                                                             \
	{                                                               \
        func();                                                     \
        AssertFail("Function should have thrown an exception");     \
	}                                                               \
    catch (const std::exception& ex)                                \
    {                                                               \
	    ExpectToBeTrue(std::strcmp((errorMsg), ex.what()) == 0)     \
    }