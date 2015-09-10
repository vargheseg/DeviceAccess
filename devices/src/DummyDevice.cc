#include <algorithm>
#include <sstream>

#include <boost/lambda/lambda.hpp>

#include "DummyDevice.h"

#include "NotImplementedException.h"
#include "MapFileParser.h"

//macro to avoid code duplication
#define TRY_REGISTER_ACCESS(COMMAND)\
		try{\
			COMMAND\
		}catch( std::out_of_range & outOfRangeException ){\
			std::stringstream errorMessage;\
			errorMessage << "Invalid address offset " << address \
			<< " in bar " << static_cast<int>(bar) << "."	\
			<< "Caught out_of_range exception: " << outOfRangeException.what();\
			throw DummyDeviceException(errorMessage.str(), DummyDeviceException::INVALID_ADDRESS);\
		}

namespace mtca4u{
// Valid bar numbers are 0 to 5 , so they must be contained
// in three bits.
const unsigned int BAR_MASK = 0x7;
// the bar number is stored in bits 60 to 62
const unsigned int BAR_POSITION_IN_VIRTUAL_REGISTER = 60;

//Noting to do, intentionally empty.
DummyDevice::DummyDevice(){
}


DummyDevice::DummyDevice(std::string host, std::string interface, std::list<std::string> parameters)
: BaseDeviceImpl(host,interface,parameters)
{
#ifdef _DEBUG
	std::cout<<"dummy is connected"<<std::endl;
#endif
}

//Nothing to clean up, all objects clean up for themselves when
//they go out of scope.
DummyDevice::~DummyDevice(){
}

void DummyDevice::open()
{
#ifdef _DEBUG
	std::cout<<"open DummyDevice"<<std::endl;
#endif

	open(_interface);
}

void DummyDevice::open(const std::string &mappingFileName,
		int /* perm */, DeviceConfigBase* /* pConfig */){
	if (_opened){
		throw DummyDeviceException("Device is already open.", DummyDeviceException::ALREADY_OPEN);
	}

	_registerMapping = mapFileParser().parse(mappingFileName);
	resizeBarContents();
	_opened=true;
}

void DummyDevice::resizeBarContents(){
	std::map< uint8_t, size_t > barSizesInBytes
	= getBarSizesInBytesFromRegisterMapping();

	for (std::map< uint8_t, size_t >::const_iterator barSizeInBytesIter
			= barSizesInBytes.begin();
			barSizeInBytesIter != barSizesInBytes.end(); ++barSizeInBytesIter){

		//the size of the vector is in words, not in bytes -> convert fist
		_barContents[barSizeInBytesIter->first].resize(
				barSizeInBytesIter->second / sizeof(int32_t), 0);
	}
}

std::map< uint8_t, size_t > DummyDevice::getBarSizesInBytesFromRegisterMapping() const{
	std::map< uint8_t, size_t > barSizesInBytes;
	for (RegisterInfoMap::const_iterator mappingElementIter = _registerMapping->begin();
			mappingElementIter <  _registerMapping->end(); ++mappingElementIter ) {
		barSizesInBytes[mappingElementIter->reg_bar] =
				std::max( barSizesInBytes[mappingElementIter->reg_bar],
						static_cast<size_t> (mappingElementIter->reg_address +
								mappingElementIter->reg_size ) );
	}
	return barSizesInBytes;
}

void DummyDevice::close(){
	if (!_opened){
		throw DummyDeviceException("Device is already closed.", DummyDeviceException::ALREADY_CLOSED);
	}

	_registerMapping.reset();// reset the shared pointer
	_barContents.clear();
	_readOnlyAddresses.clear();
	_writeCallbackFunctions.clear();
	_opened=false;
}


/*void DummyDevice::writeReg(uint8_t bar, uint32_t address, int32_t data){
	if (isReadOnly(bar, address ) ){
		return;
	}
	writeRegisterWithoutCallback(bar, address, data);
	runWriteCallbackFunctionsForAddressRange(AddressRange(bar, address, sizeof(int32_t) ) );
}*/

void DummyDevice::writeRegisterWithoutCallback(uint8_t bar, uint32_t address, int32_t data){
	TRY_REGISTER_ACCESS( _barContents[bar].at(address/sizeof(int32_t)) = data; );
}

void DummyDevice::read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes){
	checkSizeIsMultipleOfWordSize( sizeInBytes );
	unsigned int wordBaseIndex = address/sizeof(int32_t);
	for (unsigned int wordIndex = 0; wordIndex < sizeInBytes/sizeof(int32_t); ++wordIndex){
		TRY_REGISTER_ACCESS( data[wordIndex] = _barContents[bar].at(wordBaseIndex+wordIndex); );
	}
}

void DummyDevice::write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes){
	checkSizeIsMultipleOfWordSize( sizeInBytes );
	unsigned int wordBaseIndex = address/sizeof(int32_t);
	for (unsigned int wordIndex = 0; wordIndex < sizeInBytes/sizeof(int32_t); ++wordIndex){
		if (isReadOnly( bar, address + wordIndex*sizeof(int32_t) ) ){
			continue;
		}
		TRY_REGISTER_ACCESS( _barContents[bar].at(wordBaseIndex+wordIndex) = data[wordIndex]; );
	}
	runWriteCallbackFunctionsForAddressRange( AddressRange(bar, address, sizeInBytes ) );
}

void DummyDevice::readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes){
	return read(bar, address, data, sizeInBytes);
}

void DummyDevice::writeDMA(uint8_t /* bar */, uint32_t /* address */, int32_t const * /* data */, size_t /* sizeInBytes */){
	throw NotImplementedException("DummyDevice::writeDMA is not implemented yet.");
}

std::string DummyDevice::readDeviceInfo(){
	std::stringstream info;
	info << "DummyDevice with mapping file " << _registerMapping->getMapFileName();
        return info.str();
}

uint64_t DummyDevice::calculateVirtualAddress( uint32_t registerOffsetInBar,
		uint8_t bar){
	return (static_cast<uint64_t>(bar & BAR_MASK) << BAR_POSITION_IN_VIRTUAL_REGISTER) |
			(static_cast<uint64_t> (registerOffsetInBar));
}

void DummyDevice::checkSizeIsMultipleOfWordSize(size_t sizeInBytes){
	if (sizeInBytes % sizeof(int32_t) ){
		throw( DummyDeviceException("Read/write size has to be a multiple of 4", DummyDeviceException::WRONG_SIZE) );
	}
}

void DummyDevice::setReadOnly(uint8_t bar, uint32_t address,  size_t sizeInWords){
	for (size_t i = 0; i < sizeInWords; ++i){
		uint64_t virtualAddress = calculateVirtualAddress( address + i*sizeof(int32_t), bar );
		_readOnlyAddresses.insert(virtualAddress);
	}
}

void DummyDevice::setReadOnly( AddressRange addressRange ){
	setReadOnly( addressRange.bar, addressRange.offset, addressRange.sizeInBytes/sizeof(int32_t) );
}

bool  DummyDevice::isReadOnly( uint8_t bar, uint32_t address  ) const{
	uint64_t virtualAddress = calculateVirtualAddress( address, bar );
	return (  _readOnlyAddresses.find(virtualAddress) != _readOnlyAddresses.end() );
}

void  DummyDevice::setWriteCallbackFunction( AddressRange addressRange,
		boost::function<void(void)>  const & writeCallbackFunction ){
	_writeCallbackFunctions.insert(
			std::pair< AddressRange, boost::function<void(void)> >(addressRange, writeCallbackFunction) );
}

void  DummyDevice::runWriteCallbackFunctionsForAddressRange( AddressRange addressRange ){
	std::list< boost::function<void(void)> > callbackFunctionsForThisRange =
			findCallbackFunctionsForAddressRange(addressRange);
	for( std::list< boost::function<void(void)> >::iterator functionIter = callbackFunctionsForThisRange.begin();
			functionIter != callbackFunctionsForThisRange.end(); ++functionIter ){
		(*functionIter)();
	}
}

std::list< boost::function<void(void)> > DummyDevice::findCallbackFunctionsForAddressRange(
		AddressRange addressRange){
	// as callback functions are not sortable, we want to loop the multimap only once.
	// FIXME: If the same function is registered more than one, it may be executed multiple times

	// we only want the start address of the range, so we set size to 0
	AddressRange firstAddressInBar(addressRange.bar, 0, 0);
	AddressRange endAddress( addressRange.bar, addressRange.offset + addressRange.sizeInBytes, 0 );

	std::multimap< AddressRange, boost::function<void(void)> >::iterator startIterator =
			_writeCallbackFunctions.lower_bound( firstAddressInBar );

	std::multimap< AddressRange, boost::function<void(void)> >::iterator endIterator =
			_writeCallbackFunctions.lower_bound( endAddress );

	std::list< boost::function<void(void)> > returnList;
	for( std::multimap< AddressRange, boost::function<void(void)> >::iterator callbackIter = startIterator;
			callbackIter != endIterator; ++callbackIter){
		if (isWriteRangeOverlap(callbackIter->first, addressRange) ){
			returnList.push_back(callbackIter->second);
		}
	}

	return returnList;
}

bool DummyDevice::isWriteRangeOverlap( AddressRange firstRange, AddressRange secondRange){
	if (firstRange.bar != secondRange.bar){
		return false;
	}

	uint32_t startAddress = std::max( firstRange.offset, secondRange.offset );
	uint32_t endAddress = std::min( firstRange.offset  + firstRange.sizeInBytes,
			secondRange.offset + secondRange.sizeInBytes );

	// if at least one register is writeable there is an overlap of writeable registers
	for ( uint32_t address = startAddress; address < endAddress; address += sizeof(int32_t) ){
		if ( isReadOnly(firstRange.bar, address)==false ){
			return true;
		}
	}

	// we looped all possible registers, none is writable
	return false;
}


boost::shared_ptr<BaseDevice> DummyDevice::createInstance(std::string host, std::string interface, std::list<std::string> parameters) {
	return boost::shared_ptr<BaseDevice> ( new DummyDevice(host,interface,parameters) );
}

}// namespace mtca4u
