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
    Logger&                     m_logger;
    PersistableMap&             m_mapInstance;      ///< reference to the instance of persistable map
    uint32_t                    m_reportIntervalSec;///< interval between two statistical reports (in seconds)
};

/// @brief Parses network messages, executes corresponding
/// operations on map and prepares result messages
class CommandProcessor
        : private CommandProcessorContext
{
public:
    using ResultCallback = std::function<void(const ResultMessage& result)>;

    explicit CommandProcessor(const CommandProcessorContext& context);

    virtual ~CommandProcessor();

    void ProcessCommand(const CommandMessage& command, const ResultCallback& callback);

    void Start();

private:
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

    static const uint32_t scLockToutMs = 500;

    boost::asio::deadline_timer     m_reportTimer;
    std::map<uint8_t, PerfCounter>  m_performanceCounters;
    boost::asio::io_context::strand m_strand; ///< pretects m_performanceCounters
                                              ///< from concurrent access
};

}
