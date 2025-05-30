/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 Giso Grimm, Tobias Hegemann
 */
/*
 * ovbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MACAddressUtility.h"

#include <stdio.h>

#if defined(WIN32) || defined(UNDER_CE)
#include <windows.h>
#if defined(UNDER_CE)
#include <Iphlpapi.h>
#endif
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#elif defined(LINUX) || defined(linux)
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

long MACAddressUtility::GetMACAddress(unsigned char* result)
{
  // Fill result with zeroes
  memset(result, 0, 6);
  // Call appropriate function for each platform
#if defined(WIN32) || defined(UNDER_CE)
  return GetMACAddressMSW(result);
#elif defined(__APPLE__)
  return GetMACAddressMAC(result);
#elif defined(LINUX) || defined(linux)
  return GetMACAddressLinux(result);
#endif
  // If platform is not supported then return error code
  return -1;
}

#if defined(WIN32) || defined(UNDER_CE)

inline long MACAddressUtility::GetMACAddressMSW(unsigned char* result)
{

#if defined(UNDER_CE)
  IP_ADAPTER_INFO AdapterInfo[16];      // Allocate information
  DWORD dwBufLen = sizeof(AdapterInfo); // Save memory size of buffer
  if(GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_SUCCESS) {
    memcpy(result, AdapterInfo->Address, 6);
  } else
    return -1;
#else
  UUID uuid;
  if(UuidCreateSequential(&uuid) == RPC_S_UUID_NO_ADDRESS)
    return -1;
  memcpy(result, (char*)(uuid.Data4 + 2), 6);
#endif
  return 0;
}

#elif defined(__APPLE__)

static kern_return_t FindEthernetInterfaces(io_iterator_t* matchingServices)
{
  kern_return_t kernResult;
  CFMutableDictionaryRef matchingDict;
  CFMutableDictionaryRef propertyMatchDict;

  matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);

  if(NULL != matchingDict) {
    propertyMatchDict = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    if(NULL != propertyMatchDict) {
      CFDictionarySetValue(propertyMatchDict, CFSTR(kIOPrimaryInterface),
                           kCFBooleanTrue);
      CFDictionarySetValue(matchingDict, CFSTR(kIOPropertyMatchKey),
                           propertyMatchDict);
      CFRelease(propertyMatchDict);
    }
  }
  kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict,
                                            matchingServices);
  return kernResult;
}

static kern_return_t GetMACAddress(io_iterator_t intfIterator,
                                   UInt8* MACAddress, UInt8 bufferSize)
{
  io_object_t intfService;
  io_object_t controllerService;
  kern_return_t kernResult = KERN_FAILURE;

  if(bufferSize < kIOEthernetAddressSize) {
    return kernResult;
  }

  bzero(MACAddress, bufferSize);

  while((intfService = IOIteratorNext(intfIterator))) {
    CFTypeRef MACAddressAsCFData;

    // IONetworkControllers can't be found directly by the
    // IOServiceGetMatchingServices call, since they are hardware nubs and do
    // not participate in driver matching. In other words, registerService() is
    // never called on them. So we've found the IONetworkInterface and will get
    // its parent controller by asking for it specifically.

    // IORegistryEntryGetParentEntry retains the returned object, so release it
    // when we're done with it.
    kernResult = IORegistryEntryGetParentEntry(intfService, kIOServicePlane,
                                               &controllerService);

    if(KERN_SUCCESS != kernResult) {
      printf("IORegistryEntryGetParentEntry returned 0x%08x\n", kernResult);
    } else {
      // Retrieve the MAC address property from the I/O Registry in the form of
      // a CFData
      MACAddressAsCFData = IORegistryEntryCreateCFProperty(
          controllerService, CFSTR(kIOMACAddress), kCFAllocatorDefault, 0);
      if(MACAddressAsCFData) {
        // CFShow(MACAddressAsCFData); // for display purposes only; output goes
        // to stderr

        // Get the raw bytes of the MAC address from the CFData
        CFDataGetBytes((CFDataRef)MACAddressAsCFData,
                       CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
        CFRelease(MACAddressAsCFData);
      }

      // Done with the parent Ethernet controller object so we release it.
      (void)IOObjectRelease(controllerService);
    }

    // Done with the Ethernet interface object so we release it.
    (void)IOObjectRelease(intfService);
  }
  return kernResult;
}

long MACAddressUtility::GetMACAddressMAC(unsigned char* result)
{
  io_iterator_t intfIterator;
  kern_return_t kernResult = KERN_FAILURE;
  do {
    kernResult = ::FindEthernetInterfaces(&intfIterator);
    if(KERN_SUCCESS != kernResult)
      break;
    kernResult = ::GetMACAddress(intfIterator, (UInt8*)result, 6);
  } while(false);
  (void)IOObjectRelease(intfIterator);
  return kernResult;
}

#elif defined(LINUX) || defined(linux)

long MACAddressUtility::GetMACAddressLinux(unsigned char* result)
{
  struct ifreq ifr;
  struct ifreq* IFR;
  struct ifconf ifc;
  char buf[1024];
  // int s, i;
  // int ok = 0;
  bool success = false;

  auto s = socket(AF_INET, SOCK_DGRAM, 0);
  if(s == -1) {
    return -1;
  }

  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  ioctl(s, SIOCGIFCONF, &ifc);

  IFR = ifc.ifc_req;
  for(ssize_t i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++) {
    strcpy(ifr.ifr_name, IFR->ifr_name);
    if(ioctl(s, SIOCGIFFLAGS, &ifr) == 0) {
      if(!(ifr.ifr_flags & IFF_LOOPBACK)) {
        if(ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
          success = true;
          break;
        }
      }
    }
  }

  shutdown(s, SHUT_RDWR);
  if(success) {
    bcopy(ifr.ifr_hwaddr.sa_data, result, 6);
  } else {
    return -1;
  }
  return 0;
}

#endif
