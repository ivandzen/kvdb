#include <filesystem>
#include <iostream>

#include "PersistableMap.hpp"

namespace kvdb
{

static const char scMainObjectName[] = "Root";
static const std::size_t scDefaultMappedFileSize = 1024 * 1024 * 64;

PersistableMap::PersistableMap(const char* filePath)
{
    if (std::filesystem::exists(filePath))
    {
        if (!std::filesystem::is_regular_file(filePath))
        {
            throw std::runtime_error("Given path is not a regular file");
        }

        m_mappedFile = std::make_shared<MappedFile>(boost::interprocess::open_copy_on_write,
                                                    filePath);
    }
    else
    {
        m_mappedFile = std::make_shared<MappedFile>(boost::interprocess::create_only,
                                                    filePath,
                                                    scDefaultMappedFileSize);
    }

    m_allocator = std::make_unique<Allocator<void>>(m_mappedFile->get_segment_manager());


    m_internalStorage = m_mappedFile->find_or_construct<InternalStorage>(scMainObjectName)(*m_allocator);
}

PersistableMap::~PersistableMap()
{
    m_mappedFile->flush();
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

}// namespace kvdb
