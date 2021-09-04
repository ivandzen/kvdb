#pragma once

#include <string>
#include <memory>

#include <boost/fusion/sequence/io.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/adapted.hpp>

namespace kvdb
{

using boost::fusion::operators::operator>>; // for input
using boost::fusion::operators::operator<<; // for output

struct MessageHeader
{
    using Ptr = std::shared_ptr<MessageHeader>;

    static const uint32_t       scMagicInt = 0x0A0B0C0D;

    bool IsValid() const
    {
        return m_magicInt == scMagicInt;
    }

    // default constructor creates invalid header
    MessageHeader()
        : m_magicInt(0)
        , m_msgSize(0)
    {}

    explicit MessageHeader(const uint32_t size)
        : m_msgSize(size)
    {}

    uint32_t    m_magicInt = scMagicInt;
    uint32_t    m_msgSize = 0;
};

static const std::size_t scMessageHeaderSize = sizeof(MessageHeader);

/// @brief Generalized command
struct CommandMessage
{
   enum Type
   {
      INSERT,
      UPDATE,
      DELETE,
      GET
   };

   bool operator==(const CommandMessage& other) const
   {
       return type == other.type
               && key == other.key
               && value == other.value;
   }

   uint8_t      type;
   std::string  key;
   std::string  value;
};

/// @brief Command execution result
struct ResultMessage
{
   enum Code
   {
      UnknownCommand        = 0,
      WrongCommandFormat    = 1,
      InsertSuccess         = 2,
      InsertFailed          = 3,
      UpdateSuccess         = 4,
      UpdateFailed          = 5,
      GetSuccess            = 6,
      GetFailed             = 7,
      DeleteSuccess         = 8,
      DeleteFailed          = 9,
   };

   Code        code;
   std::string value;
};

}

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
      (kvdb::ResultMessage::Code, code)
      (std::string, value)
)
