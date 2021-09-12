#include <filesystem>
#include <iostream>

#include "PersistableMap.hpp"

namespace kvdb
{

static const char scMainObjectName[] = "Root";
static const std::size_t scDefaultMappedFileSize = 1024 * 1024 * 5;

PersistableMap::PersistableMap(Logger& logger)
    : m_logger(logger)
{
}

PersistableMap::~PersistableMap()
{
    if (!m_mappedFile->flush())
    {
        m_logger.LogRecord("Failed to flush map content on disk");
    }
    else
    {
        m_logger.LogRecord("Map content flushed on disk");
    }

    m_logger.LogRecord("PersistableMap destroyed");
}

void PersistableMap::InitStorage(const std::string& filePath)
{
    m_filePath = filePath;
    m_mappedFile = std::make_shared<MappedFile>(boost::interprocess::open_or_create,
                                                m_filePath.c_str(),
                                                scDefaultMappedFileSize);

    m_allocator = std::make_unique<Allocator<void>>(m_mappedFile->get_segment_manager());


    m_internalStorage = m_mappedFile->find_or_construct<InternalStorage>(scMainObjectName)(*m_allocator);
}

bool PersistableMap::Flush()
{
    std::unique_lock lock(m_mutex);
    return m_mappedFile->flush();
}

bool PersistableMap::Grow()
{
    std::unique_lock lock(m_mutex);
    if (!m_mappedFile->flush())
    {
        return false;
    }

    // reset allocator and mapped file to be able to grow it
    m_allocator.reset();
    const auto newSize = m_mappedFile->get_segment_manager()->get_size() * 2;
    m_mappedFile.reset();

    // trying to grow mapped file and reinitialize allocator
    if (!boost::interprocess::managed_mapped_file::grow(m_filePath.c_str(), newSize))
    {
        InitStorage(m_filePath);
        return false;
    }

    InitStorage(m_filePath);
    return true;
}

void PersistableMap::Insert(const std::string& key, const std::string& value, const Millis& lockTout)
{
    std::unique_lock lock(m_mutex, lockTout);
    if (!lock.owns_lock())
    {
        throw std::runtime_error("Failed to aquire unique lock on mutex");
    }

    auto& index = m_internalStorage->get<Entry::ByKey>();
    if (index.find(key) != index.end())
    {
        throw std::runtime_error("Key already exist");
    }

    index.insert(Entry(key, value, *m_allocator));
}

void PersistableMap::Update(const std::string& key, const std::string& value, const Millis& lockTout)
{
    std::unique_lock lock(m_mutex, lockTout);
    if (!lock.owns_lock())
    {
        throw std::runtime_error("Failed to aquire unique lock on mutex");
    }

    auto& index = m_internalStorage->get<Entry::ByKey>();
    auto it = index.find(key);
    if (it == index.end())
    {
        throw std::runtime_error("Key not found");
    }

    if (!index.modify(it, [&value](Entry& entry) { entry.value = value; }))
    {
        throw std::runtime_error("Failed to set value");
    }
}

void PersistableMap::Get(const std::string& key, std::string& output, const Millis& lockTout) const
{
    // several threads can access Get method
    std::shared_lock lock(m_mutex, lockTout);
    if (!lock.owns_lock())
    {
        throw std::runtime_error("Failed to aquire shared lock on mutex");
    }

    auto& index = m_internalStorage->get<Entry::ByKey>();
    auto it = index.find(key);
    if (it == index.end())
    {
        throw std::runtime_error("Key not found");
    }

    output = (*it).value;
}

void PersistableMap::Delete(const std::string& key, const Millis& lockTout)
{
    std::unique_lock lock(m_mutex, lockTout);
    if (!lock.owns_lock())
    {
        throw std::runtime_error("Failed to aquire unique lock on mutex");
    }

    auto& index = m_internalStorage->get<Entry::ByKey>();
    auto it = index.find(key);
    if (it == index.end())
    {
        throw std::runtime_error("Key not found");
    }

    index.erase(it);
}

PersistableMap::Stat PersistableMap::GetStat() const
{
    std::unique_lock lock(m_mutex);

    Stat result =
    {
        m_mappedFile->get_segment_manager()->get_size(),
        m_mappedFile->get_segment_manager()->get_free_memory(),
        m_internalStorage->get<Entry::ByKey>().size()
    };

    return result;
}

}// namespace kvdb
