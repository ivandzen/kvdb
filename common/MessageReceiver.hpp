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
class MessageReceiver
        : public std::enable_shared_from_this<MessageReceiver<MessageType>>
{
public:
    using Ptr = std::shared_ptr<MessageReceiver<MessageType>>;
    using MessageCallback = std::function<void(const MessageType& message)>;
    using WeakSelf = std::weak_ptr<MessageReceiver<MessageType>>;

    void RegisterCallback(const MessageCallback& callback)
    {
        m_callback = callback;
    }

private:
    using HeaderPtr = std::unique_ptr<MessageHeader>;
    using BufPtr = std::unique_ptr<boost::asio::streambuf>;

    void startReceive()
    {
        auto headerPtr = std::make_unique<MessageHeader>();
        const WeakSelf weakSelf = this->shared_from_this();
        boost::asio::async_read(m_socket, boost::asio::mutable_buffer(headerPtr.get(), scMessageHeaderSize),
                                [weakSelf, headerPtr](const boost::system::error_code& ec, std::size_t)
        {
            if (const auto self = weakSelf.lock())
            {
                self->onHeaderReceived(ec, headerPtr);
            }
        });
    }

    void onHeaderReceived(const boost::system::error_code& ec, HeaderPtr headerPtr)
    {
        if (ec)
        {
            m_logger->LogRecord(ec.message());
            startReceive();
            return;
        }

        if (!headerPtr->IsValid())
        {
            m_logger->LogRecord("Invalid header");
            startReceive();
            return;
        }

        const auto sbuf = std::make_unique<boost::asio::streambuf>(headerPtr->m_msgSize);
        const WeakSelf weakSelf = this->shared_from_this();
        boost::asio::async_read(m_socket, *sbuf, boost::asio::transfer_at_least(headerPtr->m_msgSize),
                                [weakSelf, sbuf](const boost::system::error_code& ec, std::size_t)
        {
            if (const auto self = weakSelf.lock())
            {
                self->onDataReceived(ec, sbuf);
            }
        });

        // start waiting for data
        m_timer.expires_from_now(boost::posix_time::milliseconds(m_dataToutMs));
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
            m_socket.cancel();
            return;
        }

        if (ec == boost::asio::error::operation_aborted)
        {
            // data succesfully received - do nothing
            return;
        }

        m_logger->LogRecord("Unexpected error occured in deadline_timer");
    }

    void onDataReceived(const boost::system::error_code& ec, BufPtr sbuf)
    {
        // cancel waiting for timeout
        m_timer.cancel();

        if (ec)
        {
            // error occured when receiving data - try to receive next message
            m_logger->LogRecord(ec.message());
            startReceive();
            return;
        }

        // data successfully received
        std::istream is(sbuf.get());
        MessageType msg;
        if (!Deserialize(is, msg))
        {
            m_logger->LogRecord("Failed to deserialize message");
        }
        else
        {
            m_callback(msg);
        }

        startReceive();
    }

    uint32_t                        m_dataToutMs;
    boost::asio::deadline_timer     m_timer;
    MessageCallback                 m_callback;
    boost::asio::ip::tcp::socket    m_socket;
    Logger::Ptr                     m_logger;
};

}
