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
                                     PerfCounter("Number of unknown commands")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::WrongCommandFormat,
                                     PerfCounter("Number of wrong format commands")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::InsertSuccess,
                                     PerfCounter("INSERT Ok")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::InsertFailed,
                                     PerfCounter("INSERT Failed")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::UpdateSuccess,
                                     PerfCounter("UPDATE Ok")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::UpdateFailed,
                                     PerfCounter("UPDATE Failed")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::GetSuccess,
                                     PerfCounter("GET Ok")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::GetFailed,
                                     PerfCounter("GET Failed")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::DeleteSuccess,
                                     PerfCounter("DELETE Ok")
                                 });
    m_performanceCounters.insert({
                                     ResultMessage::DeleteFailed,
                                     PerfCounter("DELETE Failed")
                                 });

    scheduleNextPerfornamceReport();
}

void CommandProcessor::ProcessCommand(const CommandMessage& command,
                                      const ResultCallback& callback)
{
    const auto self = shared_from_this();
    m_strand.post([self, command, callback]()
    {
        self->processCommandImpl(command, callback);
    });
}

void CommandProcessor::processCommandImpl(const CommandMessage& command,
                                          const ResultCallback& callback)
{
    ResultMessage result;
    ResultMessage::Code code;

    switch (command.type)
    {
    case CommandMessage::INSERT:
    {
        if (command.key.empty() || command.value.empty())
        {
            code = ResultMessage::WrongCommandFormat;
            break;
        }

        code = m_mapInstance->Insert(command.key, command.value)
               ? ResultMessage::InsertSuccess
               : ResultMessage::InsertFailed;
        break;
    }

    case CommandMessage::UPDATE:
    {
        if (command.key.empty() || command.value.empty())
        {
            code = ResultMessage::WrongCommandFormat;
            break;
        }

        code = m_mapInstance->Update(command.key, command.value)
               ? ResultMessage::UpdateSuccess
               : ResultMessage::UpdateFailed;
        break;
    }

    case CommandMessage::GET:
    {
        if (command.key.empty() || !command.value.empty())
        {
            code = ResultMessage::WrongCommandFormat;
            break;
        }

        code = m_mapInstance->Get(command.key, result.value)
               ? ResultMessage::GetSuccess
               : ResultMessage::GetFailed;
        break;
    }

    case CommandMessage::DELETE:
    {
        if (command.key.empty() || !command.value.empty())
        {
            code = ResultMessage::WrongCommandFormat;
            break;
        }

        code = m_mapInstance->Delete(command.key)
               ? ResultMessage::DeleteSuccess
               : ResultMessage::DeleteFailed;
        break;
    }

    default:
    {
        code = ResultMessage::UnknownCommand;
        break;
    }
    }

    ++m_performanceCounters[result.code];
    callback(result);
}

void CommandProcessor::scheduleNextPerfornamceReport()
{
    const WeakSelf weakSelf = shared_from_this();
    m_reportTimer.expires_from_now(boost::posix_time::seconds(m_reportIntervalSec));
    m_reportTimer.async_wait([weakSelf](const boost::system::error_code& ec)
    {
        // timer events and ProcessCommand calls can occur on a different threads
        if (const auto self = weakSelf.lock())
        {
            // guard shared data by executing logic in the strand
            self->m_strand.post([self, ec]()
            {
                self->onReportTimerElapsed(ec);
            });
        }
    });
}

void CommandProcessor::onReportTimerElapsed(const boost::system::error_code& ec)
{
    if (ec)
    {
        m_logger->LogRecord((boost::format("Timer error occured: %1%") % ec).str());
    }
    else
    {
        reportPerformance();
        scheduleNextPerfornamceReport();
    }
}

void CommandProcessor::reportPerformance()
{
    std::string message("================== PERFORMANCE REPORT ==================\n");
    for (const auto& entry : m_performanceCounters)
    {
        auto& counter = entry.second;
        message += (boost::format("   %1%:\t %2%\n") % counter.m_name % counter.m_counter).str();
    }
    m_logger->LogRecord(message);
}

} // namespace kvdb
