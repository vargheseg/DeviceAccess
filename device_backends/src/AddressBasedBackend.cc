/*
 * AddressBasedBackend.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: Martin Hierholzer
 */

#include "AddressBasedBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  AddressBasedBackend::AddressBasedBackend(std::string mapFileName) {
    if(mapFileName != "") {
      MapFileParser parser;
      _registerMap = parser.parse(mapFileName);
    }
    else {
      _registerMap = boost::shared_ptr<RegisterInfoMap>();
    }
  }

/********************************************************************************************************************/

  void AddressBasedBackend::read(const std::string &regModule, const std::string &regName,
      int32_t *data, size_t dataSize, uint32_t addRegOffset) {

    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    read(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  void AddressBasedBackend::write(const std::string &regModule, const std::string &regName,
      int32_t const *data, size_t dataSize, uint32_t addRegOffset) {

    uint32_t retDataSize;
    uint32_t retRegOff;
    uint8_t retRegBar;

    checkRegister(regName, regModule, dataSize, addRegOffset, retDataSize,
        retRegOff, retRegBar);
    write(retRegBar, retRegOff, data, retDataSize);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<mtca4u::RegisterAccessor> AddressBasedBackend::getRegisterAccessor(
      const std::string &registerName,
      const std::string &module) {

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(registerName, registerInfo, module);
    return boost::shared_ptr<RegisterAccessor>(
        new AddressBasedRegisterAccessor(registerInfo, boost::static_pointer_cast<DeviceBackend>(shared_from_this()) ));
  }

  /********************************************************************************************************************/

  boost::shared_ptr<const RegisterInfoMap> AddressBasedBackend::getRegisterMap() const {
    return _registerMap;
  }

  /********************************************************************************************************************/

  std::list<mtca4u::RegisterInfoMap::RegisterInfo> AddressBasedBackend::getRegistersInModule(
      const std::string &moduleName) const {
    return _registerMap->getRegistersInModule(moduleName);
  }

  /********************************************************************************************************************/

  std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > AddressBasedBackend::getRegisterAccessorsInModule(
      const std::string &moduleName) {

    std::list<RegisterInfoMap::RegisterInfo> registerInfoList =
        _registerMap->getRegistersInModule(moduleName);

    std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > accessorList;
    for (std::list<RegisterInfoMap::RegisterInfo>::const_iterator regInfo =
        registerInfoList.begin();
        regInfo != registerInfoList.end(); ++regInfo) {
      accessorList.push_back( boost::shared_ptr<mtca4u::RegisterAccessor>(
          new AddressBasedRegisterAccessor(*regInfo, boost::static_pointer_cast<DeviceBackend>(shared_from_this()) )
      ) );
    }

    return accessorList;
  }

  /********************************************************************************************************************/

  void AddressBasedBackend::checkRegister(const std::string &regName,
      const std::string &regModule, size_t dataSize,
      uint32_t addRegOffset, uint32_t &retDataSize,
      uint32_t &retRegOff, uint8_t &retRegBar) const {

    RegisterInfoMap::RegisterInfo registerInfo;
    _registerMap->getRegisterInfo(regName, registerInfo, regModule);
    if (addRegOffset % 4) {
      throw DeviceException("Register offset must be divisible by 4", DeviceException::EX_WRONG_PARAMETER);
    }
    if (dataSize) {
      if (dataSize % 4) {
        throw DeviceException("Data size must be divisible by 4", DeviceException::EX_WRONG_PARAMETER);
      }
      if (dataSize > registerInfo.nBytes - addRegOffset) {
        throw DeviceException("Data size exceed register size", DeviceException::EX_WRONG_PARAMETER);
      }
      retDataSize = dataSize;
    } else {
      retDataSize = registerInfo.nBytes;
    }
    retRegBar = registerInfo.bar;
    retRegOff = registerInfo.address + addRegOffset;
  }

} // namespace mtca4u
