#include <errno.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <sys/mman.h>

//#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

// the io constants and struct for the driver
// FIXME: they should come from the installed driver
#include "UioBackend.h"

namespace ChimeraTK {

    UioBackend::UioBackend(std::string deviceName, std::string mapFileName) : NumericAddressedBackend(mapFileName),
    _deviceID(0), _deviceMemBase(nullptr), _deviceMemSize(0), _deviceName(deviceName), _deviceNodeName(std::string()),
    _deviceSysfsPathName(std::string()) {
    }

    UioBackend::~UioBackend() {
        close();
    }

    void UioBackend::UioFindByName() {
        std::string name;

        std::string path = "/sys/class/uio/";
        for (const auto & entry : std::filesystem::directory_iterator(path)) {
            std::ifstream file(entry.path().string() + "/name");
            std::getline(file, name);
            if (name == _deviceName) {
                _deviceSysfsPathName = entry.path().string();
                _deviceNodeName = "/dev/" + entry.path().string().erase(entry.path().string().find(path), path.size());
                return;
            }
        }
        throw ChimeraTK::logic_error("Device not found");
    }

    void UioBackend::UioMMap() {
        std::string mem_size;
        std::ifstream file(_deviceSysfsPathName + "/size");
        std::getline(file, mem_size);
        _deviceMemSize = std::stoi(mem_size);
        if (MAP_FAILED == (_deviceMemBase = mmap(NULL, _deviceMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, _deviceID, 0))) {
            ::close(_deviceID);
            throw ChimeraTK::runtime_error(createErrorStringWithErrnoText("Cannot allocate Memory: "));
        }
        return;
    }

    void UioBackend::UioUnmap() {
        munmap(_deviceMemBase, _deviceMemSize);
    }

    void UioBackend::open() {
#ifdef _DEBUG
        std::cout << "open uio dev" << std::endl;
#endif
        if (_opened) {
            throw ChimeraTK::logic_error("Device already has been opened");
        }
        UioFindByName();
        _deviceID = ::open(_deviceNodeName.c_str(), O_RDWR);
        if (_deviceID < 0) {
            throw ChimeraTK::runtime_error(createErrorStringWithErrnoText("Cannot open device: "));
        }
        UioMMap();
        _opened = true;
        _interruptWaitingThread = std::thread(&UioBackend::interruptWaitingLoop, this);
        _interruptWaitingThread.detach();
    }

    void UioBackend::close() {
        if (_opened) {
            UioUnmap();
            ::close(_deviceID);
        }
        _opened = false;
    }

    void UioBackend::read(uint8_t bar, uint32_t address, int32_t* data, size_t sizeInBytes) {
        if (_opened == false) {
            throw ChimeraTK::logic_error("Device closed");
        }
        if (address + sizeInBytes > _deviceMemSize) {
            throw ChimeraTK::logic_error("Read request exceed Device Memory Region");
        }
        std::memcpy(data, _deviceMemBase + address, sizeInBytes);
    }

    void UioBackend::write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) {
        if (_opened == false) {
            throw ChimeraTK::logic_error("Device closed");
        }
        if (address + sizeInBytes > _deviceMemSize) {
            throw ChimeraTK::logic_error("Read request exceed Device Memory Region");
        }
        std::memcpy(_deviceMemBase + address, data, sizeInBytes);
    }

    std::string UioBackend::readDeviceInfo() {
        if (!_opened) throw ChimeraTK::logic_error("Device not opened.");
        std::ostringstream os;
        os << "Uio Device: " << _deviceNodeName;
        return os.str();
    }

    std::string UioBackend::createErrorStringWithErrnoText(std::string const& startText) {
        char errorBuffer[255];
        return startText + _deviceNodeName + ": " + strerror_r(errno, errorBuffer, sizeof (errorBuffer));
    }

    boost::shared_ptr<DeviceBackend> UioBackend::createInstance(std::string address,
            std::map<std::string, std::string>
            parameters) {
        if (address.size() == 0) {
            throw ChimeraTK::logic_error("Device Name not specified.");
        }

        return boost::shared_ptr<DeviceBackend>(new UioBackend(address, parameters["map"]));
    }

    void UioBackend::interruptWaitingLoop() {
        int32_t interruptWord;   
        if (false == _catalogue.hasRegister("INTERRUPT_WORD")) {
            return;
        }
        boost::shared_ptr<RegisterInfo> info = getRegisterInfo("INTERRUPT_WORD");
        auto registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);                
        uint32_t interruptWordAddress = registerInfo->address;

        while (_opened) {// only works with detached thread
            // FIXME: We need an interruption point here, otherwise the blocking read will 
            // never free the loop if there is no interrupt
            // This is the blocking read which signals the interrupt
            int dummy;
            ::read(_deviceID, &dummy, sizeof(int));

            read(0 /*bar*/, interruptWordAddress, &interruptWord, sizeof (int32_t));
            
            if (!accessorLists.empty()) {                
                for (auto & accessorList : accessorLists) {
                    int i = accessorList.first;
                    uint32_t iMask = 1 < i;
                    if (iMask & interruptWord & !accessorList.second.empty()) {
                        for (auto & accessor : accessorList.second) {
                            accessor->send();
                        }
                    }
                }                
            }
        }
    }

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> UioBackend::getRegisterAccessor_impl(
            const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
        if (registerPathName.startsWith("/INTERRUPT/")) 
        {
                return getInterruptWaitingAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
        } 
        else 
        {

            NumericAddressedBackend::getRegisterAccessor_impl<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
        }
    }

    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> UioBackend::getInterruptWaitingAccessor(
            const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
        boost::shared_ptr<NDRegisterAccessor < UserType>> accessor;

                accessor = boost::shared_ptr<NDRegisterAccessor < UserType >> 
                        (new InterruptWaitingAccessor_impl<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags));

                boost::shared_ptr<RegisterInfo> info = getRegisterInfo(registerPathName);
                auto registerInfo = boost::static_pointer_cast<RegisterInfoMap::RegisterInfo>(info);  
                                
                // detemine index from RegisterPath
                int i = (registerInfo->registerAccess) >> 2;
                accessorLists[i].push_back(accessor);
        return accessor;
    }


} // namespace ChimeraTK
