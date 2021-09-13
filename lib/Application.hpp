#pragma once

#include <boost/asio.hpp>

#include "Logger.hpp"

namespace kvdb
{

class Application
{
public:
    void Run(uint numThreads);

protected:
    boost::asio::io_context m_ioContext;
    Logger                  m_logger;

private:
    void onSystemSignal(const boost::system::error_code& error, int signalNumber);
};

}// namespace kvdb
