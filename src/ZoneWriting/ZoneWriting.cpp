#include "ZoneWriting.h"

#include "Game/IW4/ZoneWriterFactoryIW4.h"
#include "Game/T6/ZoneWriterFactoryT6.h"
#include "Writing/IZoneWriterFactory.h"

IZoneWriterFactory* ZoneWriterFactories[]
{
    new IW4::ZoneWriterFactory(),
    new T6::ZoneWriterFactory()
};

bool ZoneWriting::WriteZone(std::ostream& stream, Zone* zone)
{
    std::unique_ptr<ZoneWriter> zoneWriter;
    for (auto* factory : ZoneWriterFactories)
    {
        if(factory->SupportsZone(zone))
        {
            zoneWriter = factory->CreateWriter(zone);
            break;
        }
    }

    if (zoneWriter == nullptr)
    {
        printf("Could not create ZoneWriter for zone '%s'.\n", zone->m_name.c_str());
        return false;
    }

    return zoneWriter->WriteZone(stream);
}
