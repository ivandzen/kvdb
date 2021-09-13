#include <string>
#include <thread>
#include <vector>

#include <boost/format.hpp>

#include "Application.hpp"

namespace kvdb
{

void Application::Run(uint numThreads)
{
    boost::asio::signal_set signals(m_ioContext, SIGINT, SIGTERM);
    signals.async_wait(std::bind(&Application::onSystemSignal, this,
                                 std::placeholders::_1,
                                 std::placeholders::_2));

    const auto routine = [this]()
    {
        try
        {
            m_ioContext.run();
        }
        catch (std::exception e)
        {
            m_logger.LogRecord((boost::format("Exception: %1%") % e.what()).str());
            exit(-1);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (auto i = 0; i < numThreads; ++i)
    {
        threads.push_back(std::thread(routine));
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

void Application::onSystemSignal(const boost::system::error_code& error, int signalNumber)
{
    if (!error)
    {
        m_logger.LogRecord((boost::format("Signal %1% occured") % signalNumber).str());
        if (signalNumber == SIGINT || signalNumber == SIGTERM)
        {
            m_logger.LogRecord("Terminating...");
            m_ioContext.stop();
        }

        return;
    }

    throw std::runtime_error((boost::format("Error occured while waiting for system signal: %1%")
                              % error.message()).str());
}

}// namespace kvdb
