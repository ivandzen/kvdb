#pragma once

#include <string>
#include <iostream>

#include <boost/fusion/sequence/io.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/fusion/algorithm.hpp>

#include "Protocol.hpp"

BOOST_FUSION_ADAPT_STRUCT
(
      kvdb::CommandMessage,
      (uint8_t, type)
      (std::string, key)
      (std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT
(
      kvdb::ResultMessage,
      (uint8_t, code)
      (std::string, value)
)

namespace kvdb
{

using boost::fusion::operators::operator>>; // for input
using boost::fusion::operators::operator<<; // for output

template<typename T>
void Serialize(const T& data, std::ostream& ostream)
{
    ostream << data;
}

template<>
void Serialize<std::string>(const std::string& str, std::ostream& ostream)
{
    ostream << str.size();  // number of bytes first
    ostream << ' ';         // then space
    ostream << str;         // then characters itself
}

template<typename T>
void Deserialize(std::istream& istream, T& data)
{
    istream >> data;
}

template<>
void Deserialize<std::string>(std::istream& istream, std::string& str)
{
    uint32_t size = 0;
    istream >> size;    // read size of string
    char space;
    istream.read(&space, 1);

    // protocol violation - it must be space after string size
    if (space != ' ')
    {
        throw std::runtime_error("Failed to parse string");
    }

    if (size)
    {
        str.resize(size);
        istream.read(str.data(), size);
    }
}

template<>
void Serialize<CommandMessage>(const CommandMessage& msg, std::ostream& ostream)
{
    boost::fusion::for_each(msg, [&ostream](const auto& data)
    {
        Serialize(data, ostream);
        ostream << ' ';
    });
}

template<>
void Deserialize<CommandMessage>(std::istream& istream, CommandMessage& msg)
{
    boost::fusion::for_each(msg, [&istream](auto& data)
    {
        Deserialize(istream, data);
        char space;
        istream.read(&space, 1);

        if (space != ' ')
        {
            throw std::runtime_error("Failed to parse CommandMessage");
        }
    });
}

}
