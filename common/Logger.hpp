#pragma once

#include <memory>
#include <string>

#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace kvdb
{

class Logger
{
public:
    using Ptr = std::shared_ptr<Logger>;

    void LogRecord(const std::string& record)
    {
        if (auto lr = m_logger.open_record())
        {
            boost::log::record_ostream ostream(lr);
            ostream << record;
            ostream.flush();
            m_logger.push_record(boost::move(lr));
        }
    }

private:
    boost::log::sources::logger_mt  m_logger;
};

}
