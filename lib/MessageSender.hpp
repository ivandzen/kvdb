#pragma once

#include <memory>
#include <sstream>
#include <deque>

#include <boost/asio.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/system/system_error.hpp>

#include "Protocol.hpp"
#include "Logger.hpp"

namespace kvdb
{

struct MessageSenderContext
{
    Logger&                             m_logger; ///< pointer to logger
    boost::asio::io_context::strand&    m_strand; ///< session's strand
    boost::asio::ip::tcp::socket&       m_socket; ///< reference to socket used to transmit data
};

/// @brief sends messages of type MessageType
template<typename MessageType>
class MessageSender
        : public MessageSenderContext
{
public:
    using Ptr = std::shared_ptr<MessageSender<MessageType>>;

    explicit MessageSender(const MessageSenderContext& context)
        : MessageSenderContext(context)
    {}

    virtual ~MessageSender()
    {
        this->m_logger.LogRecord("MessageSender destroyed");
    }

    void SendMessage(const MessageType& msg)
    {
        BufPtr streambuf = std::make_shared<boost::asio::streambuf>();
        std::ostream os(streambuf.get());
        Serialize(msg, os);
        this->m_strand.post([this, streambuf]()
        {
            scheduleMessageSending(streambuf);
        });
    }

private:
    using BufPtr = std::shared_ptr<boost::asio::streambuf>;
    using HeaderPtr = std::shared_ptr<MessageHeader>;

    void scheduleMessageSending(const BufPtr& sbuf)
    {
        m_messageQueue.push_back(sbuf);
        trySendNextMessage();
    }

    void trySendNextMessage()
    {
        // if some message is currently in processing or there is no messages in queue
        if (m_currentMessage || m_messageQueue.empty())
        {
            return;
        }

        m_currentMessage = m_messageQueue.front();
        m_messageQueue.pop_front();
        const auto headerPtr = std::make_shared<MessageHeader>(m_currentMessage->size());
        boost::asio::async_write(m_socket,
                                 boost::asio::const_buffer(headerPtr.get(), scMessageHeaderSize),
                                 boost::asio::transfer_exactly(scMessageHeaderSize),
                                 // pass headerPtr to ensure it will exist until write operation completed
                                 this->m_strand.wrap(std::bind(&MessageSender::onHeaderTransmitted, this,
                                                               std::placeholders::_1, headerPtr)));
    }

    void onHeaderTransmitted(const boost::system::error_code& ec,
                             const HeaderPtr&)
    {
        // connection closed
        if (ec == boost::asio::error::eof
                || ec == boost::asio::error::broken_pipe
                || ec == boost::asio::error::connection_reset)
        {
            // no more transmission
            m_messageQueue.clear();
            return;
        }
        else if (ec)
        {
            m_logger.LogRecord(std::string("Failed to transmit header: ") + ec.message());
            m_currentMessage.reset();
            trySendNextMessage();
            return;
        }

        // Send message data
        boost::asio::async_write(m_socket, *m_currentMessage.get(),
                                 this->m_strand.wrap(std::bind(&MessageSender::onDataTransmitted, this,
                                                               std::placeholders::_1)));
    }

    void onDataTransmitted(const boost::system::error_code& ec)
    {
        // connection closed
        if (ec == boost::asio::error::eof
                || ec == boost::asio::error::broken_pipe
                || ec == boost::asio::error::connection_reset)
        {
            // no more transmission
            m_messageQueue.clear();
            return;
        }
        else if (ec)
        {
            m_logger.LogRecord(std::string("Failed to transmit data: " ) + ec.message());
            m_currentMessage.reset();
            trySendNextMessage();
            return;
        }

        m_currentMessage.reset();
        trySendNextMessage();
    }

    std::deque<BufPtr>  m_messageQueue;
    BufPtr              m_currentMessage;
};

}
