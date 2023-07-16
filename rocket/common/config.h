#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H
#include <map>
#include <tinyxml/tinyxml.h>
namespace rocket
{
    class Config
    {
    public:
        Config(const char *xmlfile);
    public:
        static Config* GetGlobalConfig();
        static void SetGlobalConfig(const char* xmlfile);
        TiXmlDocument* m_xml_document;
    public:
        std::string m_log_level;
        
    };

} // namespace rocket

#endif