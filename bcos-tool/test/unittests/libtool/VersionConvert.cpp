
#include "bcos-framework/protocol/Protocol.h"
#include <bcos-tool/VersionConverter.h>
#include <bcos-utilities/Common.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::tool;
using namespace bcos::protocol;

namespace bcos
{
namespace test
{
BOOST_AUTO_TEST_CASE(testVersionConvert)
{
    BOOST_CHECK_EQUAL(toVersionNumber("3.2.3"), static_cast<protocol::BlockVersion>(0x03020300));
    BOOST_CHECK_EQUAL(toVersionNumber("3.2"), static_cast<protocol::BlockVersion>(0x03020000U));
    BOOST_CHECK_THROW(toVersionNumber("1"), InvalidVersion);
    BOOST_CHECK_THROW(toVersionNumber("2.1"), InvalidVersion);
    BOOST_CHECK_THROW(toVersionNumber("256.1"), InvalidVersion);
}

BOOST_AUTO_TEST_CASE(testVersionCompare)
{
    BOOST_CHECK(BlockVersion::V3_3_VERSION <=> BlockVersion::V3_3_VERSION == 0);
    BOOST_CHECK(BlockVersion::V3_3_VERSION <=> BlockVersion::V3_2_VERSION >= 0);
    BOOST_CHECK(BlockVersion::V3_3_VERSION <=> BlockVersion::V3_2_VERSION > 0);
    BOOST_CHECK(BlockVersion::V3_2_VERSION <=> BlockVersion::V3_3_VERSION < 0);
    BOOST_CHECK(BlockVersion::V3_1_VERSION <=> BlockVersion::V3_3_VERSION <= 0);
}

}  // namespace test
}  // namespace bcos