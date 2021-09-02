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

bool PersistableMap::Insert(const std::string& key, const std::string& value)
{
    auto& index = m_internalStorage->get<Entry::ByKey>();
    if (index.find(key) == index.end())
    {
        index.insert(Entry(key, value, *m_allocator));
        return true;
    }

    return false;
}

bool PersistableMap::Update(const std::string& key, const std::string& value)
{
    auto& index = m_internalStorage->get<Entry::ByKey>();
    auto it = index.find(key);
    if (it == index.end())
    {
        return false;
    }

    return index.modify(it, [&value](Entry& entry)
    {
        entry.value = value;
    });
}

bool PersistableMap::Get(const std::string& key, std::string& output)
{
    auto& index = m_internalStorage->get<Entry::ByKey>();
    auto it = index.find(key);
    if (it == index.end())
    {
        return false;
    }

    output = (*it).value;
    return true;
}

bool PersistableMap::Delete(const std::string& key)
{
    auto& index = m_internalStorage->get<Entry::ByKey>();
    auto it = index.find(key);
    if (it == index.end())
    {
        return false;
    }

    index.erase(it);
    return true;
}

}// namespace kvdb
