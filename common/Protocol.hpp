#pragma once

#include <string>
#include <memory>

#include <boost/fusion/adapted.hpp>

namespace kvdb
{

struct MessageHeader
{
    using Ptr = std::unique_ptr<MessageHeader>;

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

   Type         type;
   std::string  key;
   std::string  value;
};

/// @brief Command execution result
struct ResultMessage
{
   enum Code
   {
      Success = 0,
      InsertFailed = 1,
      UpdateFailed = 2,
      DeleteFailed = 3,
      GetFailed = 4
   };

   Code        code;
   std::string data;
};

}

BOOST_FUSION_ADAPT_STRUCT
(
      kvdb::CommandMessage,
      (kvdb::CommandMessage::Type, type)
      (std::string, key)
      (std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT
(
      kvdb::ResultMessage,
      (kvdb::ResultMessage::Code, code)
      (std::string, data)
)