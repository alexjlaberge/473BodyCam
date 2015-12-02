#include "pkt.hpp"

#define BOOST_TEST_MODULE "pkt_test"
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

using bodycam::Packet;
using bodycam::PacketType;
using bodycam::ParseError;
using std::uint8_t;
using std::vector;

class Fixture
{
public:
    Fixture()
    {
        raw.push_back(2);
        raw.push_back(0);
        raw.push_back(8);
        raw.push_back(2);
        raw.push_back(1);
        raw.push_back(1);
        raw.push_back(1);
        raw.push_back(1);
        raw.push_back(10);
        raw.push_back(10);

        bad.push_back(200);
        bad.push_back(0);
        bad.push_back(8);
        bad.push_back(2);
        bad.push_back(1);
        bad.push_back(1);
        bad.push_back(1);
        bad.push_back(1);
        bad.push_back(10);
        bad.push_back(10);

        pkt = Packet(raw.data(), raw.size());
    }

    Packet pkt;
    vector<uint8_t> bad, raw;
};

BOOST_AUTO_TEST_CASE(testIDParsing)
{
    Fixture f;
    BOOST_CHECK_EQUAL(8, f.pkt.getID());
}

BOOST_AUTO_TEST_CASE(testLengthParsing)
{
    Fixture f;
    BOOST_CHECK_EQUAL(f.raw.size() - 8, f.pkt.getLength());
}

BOOST_AUTO_TEST_CASE(testOffsetParsing)
{
    Fixture f;
    BOOST_CHECK_EQUAL((1 << 24) | (1 << 16) | (1 << 8) | 1, f.pkt.getOffset());
}

BOOST_AUTO_TEST_CASE(testRawLength)
{
    Fixture f;
    BOOST_CHECK_EQUAL(f.raw.size(), f.pkt.getRawLength());
}

BOOST_AUTO_TEST_CASE(testTypeParsing)
{
    Fixture f;
    BOOST_CHECK(PacketType::MJPEG == f.pkt.getType());
}

BOOST_AUTO_TEST_CASE(testParseException)
{
    Fixture f;
    BOOST_CHECK_THROW(Packet(f.bad.data(), f.bad.size()), ParseError);
}
