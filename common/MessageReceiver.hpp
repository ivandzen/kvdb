#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>

#include "Logger.hpp"
#include "Serialization.hpp"

namespace kvdb
{

template<typename MessageType>
struct MessageReceiverContext
{
    using CloseCallback = std::function<void(void)>;
    using MessageCallback = std::function<void(const MessageType& message)>;

    boost::asio::io_context&            m_ioContext;
    boost::asio::io_context::strand&    m_strand;
    boost::asio::ip::tcp::socket&       m_socket;
    Logger&                             m_logger;
    MessageCallback                     m_msgCallback;
    CloseCallback                       m_closeCallback;
    uint32_t                            m_dataToutMs = 1000; // will wait for data after header maximum 1 second
};

template<typename MessageType>
class MessageReceiver
        : private MessageReceiverContext<MessageType>
{
public:
    /// @brief Public type aliases
    using Ptr = std::shared_ptr<MessageReceiver<MessageType>>;
    using Context = MessageReceiverContext<MessageType>;

    explicit MessageReceiver(const Context& context)
        : Context(context)
        , m_timer(context.m_ioContext)
    {
    }

    virtual ~MessageReceiver()
    {
        this->m_logger.LogRecord("MessageReceiver destroyed");
    }

    void Start()
    {
        startReceive();
    }

private:
    /// @brief private type alaises
    using BufPtr = std::shared_ptr<boost::asio::streambuf>;

    void startReceive()
    {
        auto headerPtr = std::make_shared<MessageHeader>();
        boost::asio::async_read(this->m_socket, // socket
                                boost::asio::mutable_buffer(headerPtr.get(), scMessageHeaderSize),
                                this->m_strand.wrap(std::bind(&MessageReceiver::onHeaderReceived, this,
                                                              std::placeholders::_1, headerPtr)));
    }

    void onHeaderReceived(const boost::system::error_code& ec, const MessageHeader::Ptr& headerPtr)
    {
        // connection closed
        if (ec == boost::asio::error::eof
                || ec == boost::asio::error::broken_pipe
                || ec == boost::asio::error::connection_reset)
        {
            this->m_closeCallback();
            return;
        }
        else if (ec)
        {
            this->m_logger.LogRecord(std::string("Failed to receive message header: ") + ec.message());
            startReceive();
            return;
        }

        if (!headerPtr->IsValid())
        {
            this->m_logger.LogRecord("Invalid header");
            startReceive();
            return;
        }

        auto sbuf = std::make_shared<boost::asio::streambuf>(headerPtr->m_msgSize);
        boost::asio::async_read(this->m_socket, *sbuf,
                                boost::asio::transfer_exactly(headerPtr->m_msgSize),
                                this->m_strand.wrap(std::bind(&MessageReceiver::onDataReceived, this,
                                                              std::placeholders::_1, sbuf)));

        // start waiting for data
        m_timer.expires_from_now(boost::posix_time::milliseconds(this->m_dataToutMs));
        m_timer.async_wait(this->m_strand.wrap(std::bind(&MessageReceiver::onTimerEvent, this,
                                                         std::placeholders::_1)));
    }

    void onTimerEvent(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            // timeout occured - protocol violation, abort all operations on socket
            this->m_logger.LogRecord("Read header - timeout occured");
            this->m_socket.cancel();
            return;
        }

        if (ec == boost::asio::error::operation_aborted)
        {
            // data succesfully received - do nothing
            return;
        }

        this->m_logger.LogRecord(std::string("Unexpected error occured in deadline_timer : ")
                                 + ec.message());
    }

    void onDataReceived(const boost::system::error_code& ec, const BufPtr& sbuf)
    {
        // cancel waiting for timeout
        m_timer.cancel();

        // connection may be closed
        if (ec == boost::asio::error::eof
                || ec == boost::asio::error::broken_pipe
                || ec == boost::asio::error::connection_reset)
        {
            this->m_closeCallback();
            return;
        }
        else if (ec == boost::asio::error::operation_aborted)
        {
            // Timeout occured when receiving header. Start receive again
            startReceive();
            return;
        }
        else if (ec)
        {
            this->m_logger.LogRecord(std::string("Unexpected error occured : ") + ec.message());
            startReceive();
            return;
        }

        // data successfully received
        std::istream is(sbuf.get());
        MessageType msg;
        try
        {
            Deserialize(is, msg);
            this->m_msgCallback(msg);
        }
        catch (std::runtime_error& err)
        {
            this->m_logger.LogRecord(err.what());
        }

        startReceive();
    }

    boost::asio::deadline_timer     m_timer;
};

}
