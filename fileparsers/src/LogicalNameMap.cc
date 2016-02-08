/*
 * LogicalNameMap.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <libxml/xmlreader.h>

#include "LogicalNameMap.h"
#include "DeviceException.h"

namespace mtca4u {

  LogicalNameMap::LogicalNameMap(const std::string &fileName)
  : _fileName(fileName)
  {

    // name of the current logical register
    std::string currentName;

    // name of the current tag
    std::string currentTag;

    // variables needed for the xml parser
    xmlTextReaderPtr reader;
    int ret;

    // open the xmlReader for the given file
    reader = xmlReaderForFile(fileName.c_str(), NULL, 0);
    if(reader == NULL) {
      throw DeviceException("Error opening the xlmap file '"+fileName+"'.", DeviceException::CANNOT_OPEN_MAP_FILE);
    }

    // parsing loop
    ret = xmlTextReaderRead(reader);
    while(ret == 1) {

      // obtain tag name
      std::string name((const char*)xmlTextReaderConstName(reader));

      // 0th-level tag must be "logicalNameMap"
      if(xmlTextReaderDepth(reader) == 0) {
        if(name != "logicalNameMap") {
          parsingError("Expected 'logicalNameMap' tag instead of: "+name);
        }
      }
      // 1st-level tag must be "entry": create a new entry
      else if(xmlTextReaderDepth(reader) == 1) {
        if(name != "entry") {
          parsingError("Expected 'entry' tag instead of: "+name);
        }
        // get name from attribute
        const xmlChar *entryName = xmlTextReaderGetAttribute(reader, (const xmlChar*) "name");
        if(entryName == NULL) {
          parsingError("Missing name attribute of 'entry' tag.");
        }
        currentName = (const char*) entryName;
      }
      // 2nd-level tags: fill entry with 3rd-level values
      else if(xmlTextReaderDepth(reader) == 2) {
        currentTag = name;
      }
      // 3rd level: values of 2nd-level tags
      else if(xmlTextReaderDepth(reader) == 3) {
        std::string value((const char*)xmlTextReaderConstValue(reader));
        if(name != "#text") {
          parsingError("Found tag on level 3: "+name);
        }

        // get target type
        if(currentTag == "type") {
          if(value == "register") {
            _map[currentName].targetType = TargetType::REGISTER;
          }
          else if(value == "range") {
            _map[currentName].targetType = TargetType::RANGE;
          }
          else if(value == "channel") {
            _map[currentName].targetType = TargetType::CHANNEL;
          }
          else if(value == "int_constant") {
            _map[currentName].targetType = TargetType::INT_CONSTANT;
          }
          else {
            parsingError("Wrong target type: "+value);
          }
        }

        // get device alias
        else if(currentTag == "device") {
          _map[currentName].deviceName = value;
        }

        // get register name
        else if(currentTag == "register") {
          _map[currentName].registerName = value;
        }

        // get first index of range
        else if(currentTag == "index") {
          _map[currentName].firstIndex = std::stoi(value);
        }

        // get length of range
        else if(currentTag == "length") {
          _map[currentName].length = std::stoi(value);
        }

        // get channel number
        else if(currentTag == "channel") {
          _map[currentName].channel = std::stoi(value);
        }

        // get value of constant
        else if(currentTag == "value") {
          _map[currentName].value = std::stoi(value);
        }

        // unknown tag
        else {
          parsingError("Wrong tag name at level 2: "+currentTag);
        }
      }
      else {
        parsingError("Wrong depth.");
      }


      // parse next token
      ret = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
    if(ret != 0) {
      parsingError("XML syntax error.");
    }
  }

  /********************************************************************************************************************/

  const LogicalNameMap::RegisterInfo& LogicalNameMap::getRegisterInfo(const std::string &name) {
    return _map[name];
  }

  /********************************************************************************************************************/

  void LogicalNameMap::parsingError(const std::string &message) {
    throw DeviceException("Error parsing the xlmap file '"+_fileName+"': "+message, DeviceException::CANNOT_OPEN_MAP_FILE);
  }

} // namespace mtca4u