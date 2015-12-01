#ifndef BODYCAM_PKT_HPP
#define BODYCAM_PKT_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bodycam
{

/**
 * @brief Packet types
 */
enum class PacketType : std::uint8_t
{
    NONE = 0,
    GPS = 1,
    MJPEG = 2,
    TIME = 3,
    UNCOMP = 4
};

/**
 * @brief Packet parse error
 */
class ParseError
{
public:
    ParseError(const std::string &msg);

    const std::string & getMessage() const noexcept;

private:
    std::string message;
};

/**
 * @brief Packet parsing
 *
 * @details The raw packet format is:
 *
 * | offset | size | field  |
 * |      0 |    2 | length |
 * |      2 |    1 | ID     |
 * |      3 |    1 | type   |
 * |      4 |    4 | offset |
 *
 * All fields should be stored little endian. The offset is used for delivering
 * multi-packet data.
 */
class Packet
{
public:
    /**
     * @brief Does nothing
     */
    Packet() noexcept;

    /**
     * @brief Tries to get a packet out of buf
     * @param buf Raw data buffer
     * @param len Maximum length to parse
     */
    Packet(uint8_t *buf, size_t len);

    /**
     * @brief Tries to parse a packet out of buf
     * @param buf Raw data buffer
     * @param len Maximum length to parse
     */
    void parse(uint8_t *buf, size_t len);

    /**
     * @brief Return the total packet length
     */
    uint16_t getRawLength() const noexcept;

    /**
     * @brief Return the length of data included in the packet
     */
    uint16_t getLength() const noexcept;

    /**
     * @brief Get the packet identifier
     *
     * @details This is useful for multi-packet/multi-type data transfers
     */
    uint8_t getID() const noexcept;

    /**
     * @brief Get the packet type
     */
    PacketType getType() const noexcept;

    /**
     * @brief Get the offset of the data in this packet
     *
     * @details This is useful for multi-packet data transfers
     */
    uint32_t getOffset() const noexcept;

    /**
     * @brief Get the data in this packet
     */
    const std::vector<std::uint8_t> & getData() const noexcept;

private:
    std::vector<std::uint8_t> data;
    std::uint8_t id;
    std::uint16_t length;
    std::uint32_t offset;
    PacketType type;
};

}

std::ostream & operator<<(std::ostream &stream, const bodycam::PacketType t);

#endif /* BODYCAM_PKT_HPP */
