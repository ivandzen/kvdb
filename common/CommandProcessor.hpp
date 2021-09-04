#pragma once

#include <memory>

#include <boost/asio.hpp>
#include "PersistableMap.hpp"
#include "Protocol.hpp"
#include "Logger.hpp"

namespace kvdb
{

struct CommandProcessorContext
{
    boost::asio::io_context&    m_ioContext;        ///< asynschronous context
    PersistableMap::Ptr         m_mapInstance;      ///< pointer to the instance of persistable map
    uint32_t                    m_reportIntervalSec;///< interval between two statistical reports (in seconds)
    Logger::Ptr                 m_logger;
};

class CommandProcessor
        : private CommandProcessorContext
        , public std::enable_shared_from_this<CommandProcessor>
{
public:
    using ResultCallback = std::function<void(const ResultMessage& result)>;
    using Ptr = std::shared_ptr<CommandProcessor>;

    explicit CommandProcessor(const CommandProcessorContext& context);

    void ProcessCommand(const CommandMessage& command, const ResultCallback& callback);

private:
    using WeakSelf = std::weak_ptr<CommandProcessor>;

    void processCommandImpl(const CommandMessage& command, const ResultCallback& callback);

    struct PerfCounter
    {
        std::string m_name;
        uint32_t    m_counter = 0;

        explicit PerfCounter(const std::string& name = std::string())
            : m_name(name)
        {}

        void operator++()
        {
            ++m_counter;
        }
    };

    void scheduleNextPerformanceReport();

    void onReportTimerElapsed(const boost::system::error_code& ec);

    void reportPerformance();

    boost::asio::io_context::strand m_strand;
    boost::asio::deadline_timer     m_reportTimer;
    std::map<uint8_t, PerfCounter>  m_performanceCounters;
};

}
