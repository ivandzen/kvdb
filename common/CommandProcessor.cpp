#include <chrono>

#include <boost/format.hpp>

#include "CommandProcessor.hpp"

namespace kvdb
{

CommandProcessor::CommandProcessor(const CommandProcessorContext& context)
    : CommandProcessorContext(context)
    , m_strand(context.m_ioContext)
    , m_reportTimer(context.m_ioContext)
{
    m_performanceCounters.insert({
                                     ResultMessage::UnknownCommand,
                                     PerfCounter("Number of unknown commands\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::WrongCommandFormat,
                                     PerfCounter("Number of wrong format commands")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::InsertSuccess,
                                     PerfCounter("INSERT Ok\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::InsertFailed,
                                     PerfCounter("INSERT Failed\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::UpdateSuccess,
                                     PerfCounter("UPDATE Ok\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::UpdateFailed,
                                     PerfCounter("UPDATE Failed\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::GetSuccess,
                                     PerfCounter("GET Ok\t\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::GetFailed,
                                     PerfCounter("GET Failed\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::DeleteSuccess,
                                     PerfCounter("DELETE Ok\t")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::DeleteFailed,
                                     PerfCounter("DELETE Failed\t")
                                 });
}

void CommandProcessor::Start()
{
    scheduleNextPerformanceReport();
}

void CommandProcessor::ProcessCommand(const CommandMessage& command,
                                      const ResultCallback& callback)
{
    ResultMessage result;
    ResultMessage::Code code;

    const auto& key = command.key.Get();
    const auto& value = command.value.Get();
    const auto lockTout = std::chrono::milliseconds(scLockToutMs);

    try
    {
        switch (command.type)
        {
        case CommandMessage::INSERT:
        {
            // it is allowed to set empty values for keys
            if (key.empty())
            {
                result.code = ResultMessage::WrongCommandFormat;
                break;
            }

            m_mapInstance->Insert(key, value, lockTout);
            result.code = ResultMessage::InsertSuccess;
            break;
        }

        case CommandMessage::UPDATE:
        {
            // it is allowed to set empty values for keys
            if (key.empty())
            {
                result.code = ResultMessage::WrongCommandFormat;
                break;
            }

            m_mapInstance->Update(key, value, lockTout);
            result.code = ResultMessage::UpdateSuccess;
            break;
        }

        case CommandMessage::GET:
        {
            if (key.empty() || !value.empty())
            {
                result.code = ResultMessage::WrongCommandFormat;
                break;
            }

            m_mapInstance->Get(key, result.value, lockTout);
            result.code = ResultMessage::GetSuccess;
            break;
        }

        case CommandMessage::DELETE:
        {
            if (key.empty() || !value.empty())
            {
                result.code = ResultMessage::WrongCommandFormat;
                break;
            }

            m_mapInstance->Delete(key, lockTout);
            result.code = ResultMessage::DeleteSuccess;
            break;
        }

        default:
        {
            result.code = ResultMessage::UnknownCommand;
            break;
        }
        }
    }
    catch (const std::exception& e)
    {
        m_logger->LogRecord(std::string("Exception occured when performing operation on map: ") + e.what());
        switch (command.type)
        {
        case CommandMessage::INSERT:
            result.code = ResultMessage::InsertFailed;
            break;
        case CommandMessage::UPDATE:
            result.code = ResultMessage::UpdateFailed;
            break;
        case CommandMessage::GET:
            result.code = ResultMessage::GetFailed;
            break;
        case CommandMessage::DELETE:
            result.code = ResultMessage::DeleteFailed;
            break;
        }
    }

    m_strand.post([this, result]()
    {
        // protect m_performanceCounters from concurrent access
        ++m_performanceCounters[result.code];
    });

    callback(result);
}

void CommandProcessor::scheduleNextPerformanceReport()
{
    m_reportTimer.expires_from_now(boost::posix_time::seconds(m_reportIntervalSec));
    m_reportTimer.async_wait(std::bind(&CommandProcessor::onReportTimerElapsed, this,
                                       std::placeholders::_1));
}

void CommandProcessor::onReportTimerElapsed(const boost::system::error_code& ec)
{
    if (ec)
    {
        m_logger->LogRecord((boost::format("Timer error occured: %1%") % ec).str());
    }
    else
    {
        // protect m_performanceCounters from concurrent access
        m_strand.post(std::bind(&CommandProcessor::reportPerformance, this));
        scheduleNextPerformanceReport();
    }
}

void CommandProcessor::reportPerformance()
{
    std::string message("================== PERFORMANCE REPORT ==================\n");
    for (const auto& entry : m_performanceCounters)
    {
        auto& counter = entry.second;
        message += (boost::format("   %1%: %2%\n") % counter.m_name % counter.m_counter).str();
    }
    m_logger->LogRecord(message);
}

} // namespace kvdb
