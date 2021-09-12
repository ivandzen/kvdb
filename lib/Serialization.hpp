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
      (int, type)
      (kvdb::LimitedString, key)
      (kvdb::LimitedString, value)
)

BOOST_FUSION_ADAPT_STRUCT
(
      kvdb::ResultMessage,
      (int, code)
      (kvdb::LimitedString, value)
)

namespace kvdb
{

using boost::fusion::operators::operator>>; // for input
using boost::fusion::operators::operator<<; // for output

/// Default imlementations uses standard <<, >> operators

template<typename T>
inline void Serialize(const T& data, std::ostream& ostream)
{
    ostream << data;
}

template<typename T>
inline void Deserialize(std::istream& istream, T& data)
{
    istream >> data;
}

/// Specialized Serialize|Deserialize functions for std::string are needed because
/// default implementation of stream opeartors (<<, >>) are pretty simple
/// and can not correctly serialize-deserialize wide range of strings with spaces,
/// special symbols, etc.

/// Special implementation that prepends string data with total length of the string
template<>
inline void Serialize<LimitedString>(const LimitedString& str, std::ostream& ostream)
{
    ostream << str.Get().size();  // number of bytes first
    ostream << ' ';               // then space
    ostream << str.Get();         // then characters itself
}

template<>
inline void Deserialize<LimitedString>(std::istream& istream, LimitedString& str)
{
    uint32_t size = 0;
    istream >> size;    // read size of string

    // try to find space next
    char space;
    istream.read(&space, 1);

    if (space != ' ')
    {
        // protocol violation - it must be space after string size
        throw std::runtime_error("Failed to parse string");
    }

    if (size && size <= str.MaxSize())
    {
        std::string buffer;
        buffer.resize(size);
        istream.read(buffer.data(), size);
        str.Set(buffer);
    }
}

template<typename MessageType>
inline void SerializeProtocolMessage(const MessageType& msg, std::ostream& ostream)
{
    boost::fusion::for_each(msg, [&ostream](const auto& data)
    {
        Serialize(data, ostream);
        ostream << ' ';
    });
}


template<typename MessageType>
inline void DeserializeProtocolMessage(std::istream& istream, MessageType& msg)
{
    boost::fusion::for_each(msg, [&istream](auto& data)
    {
        Deserialize(istream, data);

        // try to find space after previously parsed element
        char space;
        istream.read(&space, 1);

        if (space != ' ')
        {
            // protocol violation
            throw std::runtime_error("Failed to deserialize message");
        }
    });
}

template<>
inline void Serialize<CommandMessage>(const CommandMessage& msg, std::ostream& ostream)
{
    SerializeProtocolMessage(msg, ostream);
}

template<>
inline void Deserialize<CommandMessage>(std::istream& istream, CommandMessage& msg)
{
    DeserializeProtocolMessage(istream, msg);
}

template<>
inline void Serialize<ResultMessage>(const ResultMessage& msg, std::ostream& ostream)
{
    SerializeProtocolMessage(msg, ostream);
}

template<>
inline void Deserialize<ResultMessage>(std::istream& istream, ResultMessage& msg)
{
    DeserializeProtocolMessage(istream, msg);
}

}// namespace kvdb
