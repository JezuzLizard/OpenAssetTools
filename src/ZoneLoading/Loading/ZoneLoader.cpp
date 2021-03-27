#include "ZoneLoader.h"
#include "Exception/LoadingException.h"
#include "LoadingFileStream.h"

#include <algorithm>

ZoneLoader::ZoneLoader(Zone* zone)
{
    m_zone = zone;
    m_processor_chain_dirty = false;
}

ILoadingStream* ZoneLoader::BuildLoadingChain(ILoadingStream* rootStream)
{
    auto* currentStream = rootStream;

    for(const auto& processor : m_processors)
    {
        processor->SetBaseStream(currentStream);

        currentStream = processor.get();
    }
    
    m_processor_chain_dirty = false;
    return currentStream;
}

void ZoneLoader::AddXBlock(std::unique_ptr<XBlock> block)
{
    m_blocks.push_back(block.get());

    std::sort(m_blocks.begin(), m_blocks.end(), [](XBlock* b1, XBlock* b2) -> bool
    {
        return b1->m_index < b2->m_index;
    });

    m_zone->GetMemory()->AddBlock(std::move(block));
}

void ZoneLoader::AddLoadingStep(std::unique_ptr<ILoadingStep> step)
{
    m_steps.emplace_back(std::move(step));
}

void ZoneLoader::AddStreamProcessor(std::unique_ptr<StreamProcessor> streamProcessor)
{
    m_processors.push_back(std::move(streamProcessor));
    m_processor_chain_dirty = true;
}

void ZoneLoader::RemoveStreamProcessor(StreamProcessor* streamProcessor)
{
    for(auto i = m_processors.begin(); i < m_processors.end(); ++i)
    {
        if(i->get() == streamProcessor)
        {
            m_processors.erase(i);
            m_processor_chain_dirty = true;
            break;
        }
    }
}

Zone* ZoneLoader::LoadZone(std::istream& stream)
{
    LoadingFileStream fileStream(stream);
    auto* endStream = BuildLoadingChain(&fileStream);

    try
    {
        for(const auto& step : m_steps)
        {
            step->PerformStep(this, endStream);

            if(m_processor_chain_dirty)
            {
                endStream = BuildLoadingChain(&fileStream);
            }
        }
    }
    catch (LoadingException& e)
    {
        const auto detailedMessage = e.DetailedMessage();
        printf("Loading fastfile failed: %s\n", detailedMessage.c_str());

        delete m_zone;
    
        return nullptr;
    }

    m_zone->Register();

    return m_zone;
}
