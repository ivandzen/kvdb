#pragma once

#include <functional>
#include <memory>

#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>

#include "Protocol.hpp"
#include "Logger.hpp"

namespace kvdb
{

template<typename MessageType>
struct MessageReceiverContext
{
    using MessageCallback = std::function<void(const MessageType& message)>;

    boost::asio::io_context&        m_ioContext;
    boost::asio::ip::tcp::socket&   m_socket;
    Logger::Ptr                     m_logger;
    MessageCallback                 m_callback;
    uint32_t                        m_dataToutMs = 1000; // will wait for data after header maximum 1 second
};

template<typename MessageType>
class MessageReceiver
        : public std::enable_shared_from_this<MessageReceiver<MessageType>>
        , private MessageReceiverContext<MessageType>
{
public:
    /// @brief Public type aliases
    using Ptr = std::shared_ptr<MessageReceiver<MessageType>>;
    using Context = MessageReceiverContext<MessageType>;

    explicit MessageReceiver(const Context& context)
        : Context(context)
        , m_timer(context.m_ioContext)
    {
        startReceive();
    }

private:
    /// @brief private type alaises
    using WeakSelf = std::weak_ptr<MessageReceiver<MessageType>>;
    using HeaderPtr = std::unique_ptr<MessageHeader>;
    using BufPtr = std::unique_ptr<boost::asio::streambuf>;

    void startReceive()
    {
        auto headerPtr = std::make_unique<MessageHeader>();
        const WeakSelf weakSelf = this->shared_from_this();
        boost::asio::async_read(this->m_socket, boost::asio::mutable_buffer(headerPtr.get(), scMessageHeaderSize),
                                [weakSelf, hp = move(headerPtr)](const boost::system::error_code& ec, std::size_t)
        {
            if (const auto self = weakSelf.lock())
            {
                self->onHeaderReceived(ec, move(hp));
            }
        });
    }

    void onHeaderReceived(const boost::system::error_code& ec, const HeaderPtr&& headerPtr)
    {
        if (ec)
        {
            this->m_logger->LogRecord(ec.message());
            startReceive();
            return;
        }

        if (!headerPtr->IsValid())
        {
            this->m_logger->LogRecord("Invalid header");
            startReceive();
            return;
        }

        auto sbuf = std::make_unique<boost::asio::streambuf>(headerPtr->m_msgSize);
        const WeakSelf weakSelf = this->shared_from_this();
        boost::asio::async_read(this->m_socket, *sbuf, boost::asio::transfer_at_least(headerPtr->m_msgSize),
                                [weakSelf, sb = move(sbuf)](const boost::system::error_code& ec, std::size_t)
        {
            if (const auto self = weakSelf.lock())
            {
                self->onDataReceived(ec, move(sb));
            }
        });

        // start waiting for data
        m_timer.expires_from_now(boost::posix_time::milliseconds(this->m_dataToutMs));
        m_timer.async_wait([weakSelf](const boost::system::error_code& ec)
        {
            if (const auto self = weakSelf.lock())
            {
                self->onTimerEvent(ec);
            }
        });
    }

    void onTimerEvent(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            // timeout occured - protocol violation, abort all operations on socket
            this->m_socket.cancel();
            return;
        }

        if (ec == boost::asio::error::operation_aborted)
        {
            // data succesfully received - do nothing
            return;
        }

        this->m_logger->LogRecord("Unexpected error occured in deadline_timer");
    }

    void onDataReceived(const boost::system::error_code& ec, const BufPtr&& sbuf)
    {
        // cancel waiting for timeout
        m_timer.cancel();

        if (ec)
        {
            // error occured when receiving data - try to receive next message
            this->m_logger->LogRecord(ec.message());
            startReceive();
            return;
        }

        // data successfully received
        std::istream is(sbuf.get());
        MessageType msg;

        throw std::runtime_error(std::string(__FILE__) + " : " + std::to_string(__LINE__));

        /// @todo
        //if (!Deserialize(is, msg))
        //{
        //    this->m_logger->LogRecord("Failed to deserialize message");
        //}
        //else
        //{
        //    m_callback(msg);
        //}

        startReceive();
    }

    boost::asio::deadline_timer     m_timer;
};

}
