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

#ifndef _MACADDRESS_UTILITY_H
#define _MACADDRESS_UTILITY_H

class MACAddressUtility {
public:
  static long GetMACAddress(unsigned char* result);

private:
#if defined(WIN32) || defined(UNDER_CE)
  static long GetMACAddressMSW(unsigned char* result);
#elif defined(__APPLE__)
  static long GetMACAddressMAC(unsigned char* result);
#elif defined(LINUX) || defined(linux)
  static long GetMACAddressLinux(unsigned char* result);
#endif
};

#endif
