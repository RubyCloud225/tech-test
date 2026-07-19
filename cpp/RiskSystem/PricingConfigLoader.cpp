#include "PricingConfigLoader.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

/**
 * Ensures no whitespace nor line endings are parsed within 
 * the xml file. It extracts name = "..." attribute from a single XML element.
 */
static std::string attributeValue(const std::string& element, const std::string& name) {
    const std::string key = name + "=\"";
    std::size_t start = element.find(key);
    if (start == std::string::npos) {
        throw std::runtime_error("Missing attribute: " + name);
    }
    start += key.size();
    std::size_t end = element.find('"', start);
    if (end == std::string::npos) {
        throw std::runtime_error("Malformed attribute: " + name);
    }
    return element.substr(start, end - start);
}


std::string PricingConfigLoader::getConfigFile() const {
    return configFile_;
}

void PricingConfigLoader::setConfigFile(const std::string& file) {
    configFile_ = file;
}

/**
 * parses PricingConfig/PricingEngines.xml
 * 
 * The config is parsed directly by using flat list of <Engine/> elements 
 * rather than using XML library.
 */

PricingEngineConfig PricingConfigLoader::parseXml(const std::string& content) {
    PricingEngineConfig config;
    std::size_t pos = 0;

    while ((pos = content.find("<Engine", pos)) != std::string::npos) {
        // search for single element so that the attribute value would scan each element
        // for incorrect chars.
        std::size_t end = content.find('>', pos);
        if (end == std::string::npos) {
            throw std::runtime_error("Malformed Engine element");
        }
        const std::string element = content.substr(pos, end - pos);

        PricingEngineConfigItem item;
        item.setTradeType(attributeValue(element, "tradeType"));
        item.setAssembly(attributeValue(element, "assembly"));
        // The xml attribute is named pricingEngine rather than getTypeName().
        item.setTypeName(attributeValue(element, "pricingEngine"));
        config.push_back(item);
        pos = end; // advance past this element
    }

    return config;
}


PricingEngineConfig PricingConfigLoader::loadConfig() {
    if (configFile_.empty()) {
        throw std::invalid_argument("Config file cannot be null");
    }

    std::ifstream stream(configFile_);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open file: " + configFile_);
    }

    std::stringstream buffer;
    buffer << stream.rdbuf();
    return parseXml(buffer.str());
}
