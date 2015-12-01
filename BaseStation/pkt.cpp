#include "pkt.hpp"

using bodycam::Packet;
using bodycam::PacketType;
using bodycam::ParseError;
using std::vector;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

ParseError::ParseError(const std::string &msg) : message{msg}
{
}

const std::string & ParseError::getMessage() const noexcept
{
    return message;
}

Packet::Packet() noexcept : id{0}, length{0}, offset{0}, type{PacketType::NONE}
{
}

Packet::Packet(uint8_t *buf, size_t len)
{
    parse(buf, len);
}

void Packet::parse(uint8_t *buf, size_t len)
{
    uint16_t i{0};

    if (len < 8)
    {
        throw ParseError("Length is less than 8");
    }

    length = *((uint16_t *) (buf + 0));
    id = *(buf + 2);
    type = static_cast<PacketType>(*(buf + 3));
    offset = *((uint32_t *) (buf + 4));

    if (length == 0)
    {
        throw ParseError("Length of packet is 0");
    }

    data = vector<uint8_t>(length);
    for (i = 0; i < length; i++)
    {
        data[i] = buf[i + 8];
    }
}

uint16_t Packet::getRawLength() const noexcept
{
    return getLength() + 8;
}

uint16_t Packet::getLength() const noexcept
{
    return length;
}

uint8_t Packet::getID() const noexcept
{
    return id;
}

PacketType Packet::getType() const noexcept
{
    return type;
}

uint32_t Packet::getOffset() const noexcept
{
    return offset;
}

const std::vector<std::uint8_t> & Packet::getData() const noexcept
{
    return data;
}

std::ostream & operator<<(std::ostream &stream, const PacketType t)
{
    stream << static_cast<uint8_t>(t);
    return stream;
}
