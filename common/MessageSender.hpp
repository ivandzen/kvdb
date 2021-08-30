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
    boost::asio::io_context::strand m_strand; ///< allows execution of socket callbacks and public methods from different threads
    boost::asio::ip::tcp::socket&   m_socket; ///< reference to socket used to transmit data
    Logger&                         m_logger; ///< reference to logger
};

template<typename MessageType>
class MessageSender
        : public MessageSenderContext
        , public std::enable_shared_from_this<MessageSender<MessageType>>
{
public:
    using Ptr = std::shared_ptr<MessageSender<MessageType>>;
    using WeakSelf = std::weak_ptr<MessageSender<MessageType>>;

    explicit MessageSender(const MessageSenderContext& context)
        : MessageSenderContext(context)
    {}

    void SendMessage(const MessageType& msg)
    {
        BufPtr streambuf = std::make_shared<boost::asio::streambuf>();
        std::ostream os(streambuf.get());
       // if (!Serialize(msg, os))
       // {
       //     m_logger.LogRecord("Failed to serialize message");
       //     return false;
       // }

        const auto self = this->shared_from_this();
        m_strand.post([self, streambuf]()
        {
            self->scheduleMessageSending(streambuf);
        });
    }

private:
    using BufPtr = std::shared_ptr<boost::asio::streambuf>;

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

        const WeakSelf weakPtr = this->shared_from_this();
        boost::asio::async_write(m_socket,
                                 boost::asio::const_buffer(headerPtr.get(), scMessageHeaderSize),
                                 boost::asio::transfer_at_least(scMessageHeaderSize),
                                 // pass headerPtr to ensure it will exist until write operation completed
                                 [weakPtr, headerPtr](const boost::system::error_code& ec, std::size_t)
        {
            if (const auto self = weakPtr.lock())
            {
                self->m_strand.post([self, ec]()
                {
                    self->onHeaderTransmitted(ec);
                });
            }
        });
    }

    void onHeaderTransmitted(const boost::system::error_code& ec)
    {
        if (ec)
        {
            m_logger.LogRecord(ec.message());
            m_currentMessage.reset();
            trySendNextMessage();
            return;
        }

        // Send message data
        const WeakSelf weakPtr = this->shared_from_this();
        boost::asio::async_write(m_socket, *m_currentMessage.get(),
                                 [weakPtr](const boost::system::error_code& ec, std::size_t)
        {
            if (const auto self = weakPtr.lock())
            {
                self->m_strand.post([self, ec]()
                {
                    self->onDataTransmitted(ec);
                });
            }
        });
    }

    void onDataTransmitted(const boost::system::error_code& ec)
    {
        if (ec)
        {
            m_logger.LogRecord(ec.message());
            m_currentMessage.reset();
            trySendNextMessage();
            return;
        }

        m_currentMessage.reset();
        trySendNextMessage();
    }

    std::deque<BufPtr>              m_messageQueue;
    BufPtr                          m_currentMessage;
};

}
