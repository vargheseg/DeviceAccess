/*
 * TransferGroup.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include "TransferGroup.h"
#include "BufferingRegisterAccessor.h"
#include "TwoDRegisterAccessor.h"

namespace mtca4u {

  void TransferGroup::read() {
    for(unsigned int i=0; i<elements.size(); i++) {
      elements[i]->read();
    }
  }

  /*********************************************************************************************************************/

  void TransferGroup::write() {
    if(isReadOnly()) {
      throw DeviceException("TransferGroup::write() called, but at least one TransferElement is read-only.",
          DeviceException::REGISTER_IS_READ_ONLY);
    }
    for(unsigned int i=0; i<elements.size(); i++) {
      elements[i]->write();
    }
  }

  /*********************************************************************************************************************/

  bool TransferGroup::isReadOnly() {
    return readOnly;
  }

  /*********************************************************************************************************************/

  template<>
  void TransferGroup::addAccessor<TransferElement>(TransferElement &accessor) {
    auto newElements = accessor.getHardwareAccessingElements();

    // iterate over all new elements
    for(unsigned int i=0; i<newElements.size(); i++) {

      // iterate over all elements already in the list and check if element is already in there
      bool foundDuplicate = false;
      for(unsigned int k=0; k<elements.size(); k++) {
        if(newElements[i]->isSameRegister(elements[k])) {
          // replace the TransferElement inside the accessor with the version already in the list
          accessor.replaceTransferElement(elements[k]);
          // set flag and stop the loop (the list is already unique, so no further match is possible)
          foundDuplicate = true;
          break;
        }
      }

      // if not a duplicate, add to the list
      if(!foundDuplicate) elements.push_back(newElements[i]);
    }

    // update read-only flag
    if(accessor.isReadOnly()) readOnly = true;
  }

} /* namespace mtca4u */