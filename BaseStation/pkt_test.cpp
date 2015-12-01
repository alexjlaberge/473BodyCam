#include "pkt.hpp"

#define BOOST_TEST_MODULE "pkt_test"
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

using bodycam::Packet;
using bodycam::PacketType;
using std::uint8_t;
using std::vector;

class Fixture
{
public:
    Fixture()
    {
        raw.push_back(10);
        raw.push_back(0);
        raw.push_back(8);
        raw.push_back(2);
        raw.push_back(1);
        raw.push_back(1);
        raw.push_back(1);
        raw.push_back(1);
        raw.push_back(10);
        raw.push_back(10);

        pkt = Packet(raw.data(), raw.size());
    }

    Packet pkt;
    vector<uint8_t> raw;
};

BOOST_AUTO_TEST_CASE(testIDParsing)
{
    Fixture f;
    BOOST_CHECK_EQUAL(8, f.pkt.getID());
}

BOOST_AUTO_TEST_CASE(testLengthParsing)
{
    Fixture f;
    BOOST_CHECK_EQUAL(10, f.pkt.getLength());
}
BOOST_AUTO_TEST_CASE(testOffsetParsing)
{
    Fixture f;
    BOOST_CHECK_EQUAL((1 << 24) | (1 << 16) | (1 << 8) | 1, f.pkt.getOffset());
}
BOOST_AUTO_TEST_CASE(testTypeParsing)
{
    Fixture f;
    BOOST_CHECK(PacketType::MJPEG == f.pkt.getType());
}
