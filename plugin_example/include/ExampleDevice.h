/*
 * ExampleDevice.h
 *
 *  Created on: Jul 31, 2015
 *      Author: nshehzad
 */

#ifndef SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_
#define SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_
#include "MtcaMappedDevice/BaseDeviceImpl.h"
#include "MtcaMappedDevice/DeviceConfigBase.h"
#include "MtcaMappedDevice/DeviceFactory.h"
#include <list>
#include <iostream>
#include <boost/shared_ptr.hpp>
using namespace mtca4u;

class ExampleDevice : public BaseDeviceImpl {
private:
	ExampleDevice(std::string host, std::string interface, std::list<std::string> parameters);
public:
  ExampleDevice(){};
  virtual ~ExampleDevice();
  virtual void open();
  virtual void close();
  static boost::shared_ptr<mtca4u::BaseDevice> createInstance(std::string host, std::string interface, std::list<std::string> parameters);

  virtual void open(const std::string& /*devName*/, int /*perm*/,
                         DeviceConfigBase* /*pConfig*/) {};

    virtual void read(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/,  size_t /*sizeInBytes*/){};
    virtual void write(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};

    virtual void readDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/, size_t /*sizeInBytes*/) {};
    virtual void writeDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};

    virtual std::string readDeviceInfo() {return std::string("Example_Device");}
};

class ExampleDeviceRegisterer{
public:
	ExampleDeviceRegisterer(){
#ifdef _DEBUG
		std::cout<<"ExampleDeviceRegisterer"<<std::endl;
#endif
		DeviceFactory::getInstance().registerDeviceType("exx","",&ExampleDevice::createInstance);
  }
/*	static ExampleDeviceRegisterer & init()
	{
		static ExampleDeviceRegisterer globalExampleDeviceRegisterer;
		return globalExampleDeviceRegisterer;
	}*/

};
ExampleDeviceRegisterer globalExampleDeviceRegisterer;


#endif /* SOURCE_DIRECTORY__EXAMPLES_EXAMPLEDEVICE_H_ */
