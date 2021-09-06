#pragma once

#include <string>
#include <memory>

namespace kvdb
{

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

static const uint32_t scReceiveDataTOutMs = 1000;
static const std::size_t scMessageHeaderSize = sizeof(MessageHeader);

class LimitedString
{
public:
    LimitedString(const std::size_t maxSize,
                  const std::string& str)
        : m_maxSize(maxSize)
    {
        Set(str);
    }

    void Set(const std::string& str, bool nocheck = false)
    {
        if (!nocheck)
        {
            checkString(str);
        }
        m_content = str;
    }

    const std::string& Get() const
    {
        return m_content;
    }

    std::size_t MaxSize() const
    {
        return m_maxSize;
    }

    bool operator==(const LimitedString& other) const
    {
        return m_maxSize == other.m_maxSize
                && m_content == other.m_content;
    }

private:
    void checkString(const std::string& str) const
    {
        if (str.size() > m_maxSize)
        {
            throw std::runtime_error("LimitedString size overflow");
        }
    }

    std::size_t m_maxSize;
    std::string m_content;
};

/// @brief datatype used to identify commands
using CommandID = uint32_t;

/// @brief Generalized command
struct CommandMessage
{
   enum Type
   {
      UNKNOWN,
      INSERT,
      UPDATE,
      DELETE,
      GET
   };

   static const std::size_t scMaxKeySize = 1024;
   static const std::size_t scMaxValueSize = 1024 * 1024;

   CommandMessage(const uint8_t type = 0,
                  const std::string& key = std::string(),
                  const std::string& value = std::string())
       : type(type)
       , key(scMaxKeySize, key)
       , value(scMaxValueSize, value)
   {}

   bool operator==(const CommandMessage& other) const
   {
       return type == other.type
               && key == other.key
               && value == other.value;
   }

   CommandID        id = 0;
   uint8_t          type = UNKNOWN;
   LimitedString    key;
   LimitedString    value;
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

   CommandID    commandId = 0;
   uint8_t      code;
   std::string  value;
};

}
