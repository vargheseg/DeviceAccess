#ifndef MTCA4U_LIBDEV_FAKE_H
#define MTCA4U_LIBDEV_FAKE_H

#include "BaseDevice.h"
#include "BaseDeviceImpl.h"
#include <stdint.h>
#include <stdlib.h>
#include <boost/shared_ptr.hpp>

#define MTCA4U_LIBDEV_BAR_NR 8
#define MTCA4U_LIBDEV_BAR_MEM_SIZE (1024 * 1024)

namespace mtca4u {

class FakeDevice : public BaseDeviceImpl {
private:
  FILE* _pcieMemory;
  std::string _pcieMemoryFileName;
  FakeDevice(std::string host, std::string interface, std::list<std::string> parameters);
public:
  FakeDevice();
  virtual ~FakeDevice();

  virtual void open(const std::string& devName, int perm = O_RDWR,
                       DeviceConfigBase* pConfig = NULL);

  virtual void open();
  virtual void close();

  virtual void readRaw(uint32_t regOffset, int32_t* data, uint8_t bar);
  virtual void writeRaw(uint32_t regOffset, int32_t data, uint8_t bar);

  virtual void readArea(uint32_t regOffset, int32_t* data, size_t size,
                        uint8_t bar);
  virtual void writeArea(uint32_t regOffset, int32_t const* data, size_t size,
                         uint8_t bar);

  virtual void readDMA(uint32_t regOffset, int32_t* data, size_t size,
                       uint8_t bar);
  virtual void writeDMA(uint32_t regOffset, int32_t const* data, size_t size,
                        uint8_t bar);

  virtual std::string readDeviceInfo();

  static boost::shared_ptr<BaseDevice> createInstance(std::string host, std::string interface, std::list<std::string> parameters);

private:
  /// A private copy constructor, cannot be called from outside.
  /// As the default is not safe and I don't want to implement it right now, I
  /// just make it
  /// private. Make sure not to use it within the class before writing a proper
  /// implemenFakeDevice
  FakeDevice(FakeDevice const&) : BaseDeviceImpl(), _pcieMemory(0), _pcieMemoryFileName() {}

  /// A private assignment operator, cannot be called from outside.
  /// As the default is not safe and I don't want to implement it right now, I
  /// just make it
  /// private. Make sure not to use it within the class before writing a proper
  /// imFakeDeviceation.
  FakeDevice& operator=(FakeDevice const&) { return *this; }
};

} // namespace mtca4u

#endif /* MTCA4U_LIBDEV_STRUCT_H */
