#include <mtca4u/DummyBackend.h>
#include <mtca4u/PcieBackend.h>
#include <mtca4u/DeviceBackendImpl.h>
#include <mtca4u/Device.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// For the simple example the offset of the user word is hard coded.
static const unsigned int WORD_USER_OFFSET = 0xC;
static const unsigned int WORD_USER_BAR = 0;

int main(){
  // Before you use a device you have to tell the factory 
  // which dmap file to use.
  // \todo Fixme: we use one from the unit tests. examples should have its own
  // \todo There should be a global function to do this. It is an implementation
  // detail that it's the factory which has to know it.
  mtca4u::BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);

  boost::shared_ptr<mtca4u::Device> myDevice( new mtca4u::Device());
  myDevice->open("PCIE1");
  // read and print a data word from a register
  int32_t dataWord;
  myDevice->readReg(WORD_USER_OFFSET, &dataWord, 0 /*bar 0*/);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  // write something different to the register, read it back and print it
  myDevice->writeReg(WORD_USER_OFFSET, dataWord + 42, 0 /*bar 0*/);
  myDevice->readReg(WORD_USER_OFFSET, &dataWord, 0 /*bar 0*/);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice->close();

  return 0;
}