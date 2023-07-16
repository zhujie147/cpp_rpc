#include "rocket/common/config.h"


#define READ_XML_NODE(name, parent)                                           \
    TiXmlElement *name##_node = parent->FirstChildElement(#name);             \
    if (!(name##_node))                                                       \
    {                                                                         \
        printf("Start rocket server error, failed to read node [%s]", #name); \
        exit(0);                                                              \
    }

#define READ_STR_FROM_XML_NODE(name, parent)                                        \
    TiXmlElement *name##_node = parent->FirstChildElement(#name);        \
    if (!name##_node || !name##_node->GetText())                              \
    {                                                                               \
        printf("Start rocket server error, failed to read node [%s]", #name); \
        exit(0);                                                                    \
    }                                                                               \
    std::string name##_str = std::string(name##_node->GetText());

namespace rocket
{
    static Config* g_config=NULL;
    Config* Config::GetGlobalConfig(){
        return g_config;
    }

    void Config::SetGlobalConfig(const char* xmlfile){
        if(g_config==NULL){
            g_config=new Config(xmlfile);
        }
        
    }

    Config::Config(const char *xmlfile)
    {
        m_xml_document = new TiXmlDocument();

        //read config file
        bool rt = m_xml_document->LoadFile(xmlfile);
        if (!rt)
        {
            printf("Start rocket server error, failed to config file %s", xmlfile);
            exit(0);
        }

        READ_XML_NODE(root, m_xml_document);
        READ_XML_NODE(log, root_node);

        READ_STR_FROM_XML_NODE(log_level,log_node);
        m_log_level=log_level_str;
    }

}