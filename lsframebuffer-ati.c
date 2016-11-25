// Copyright (C) 2012 - sfinktah, but mainly i stole code and comments
// from apple and random forum posts, so i don't really own the rights
// to jack shit.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// lsframebuffer.c
// compile with something similar to this line:
//    gcc -mmacosx-version-min=10.5 -Wall -o lsframebuffer-ati lsframebuffer-ati.c -framework IOKit -framework ApplicationServices

#include <stdio.h>
#include <strings.h>
#include <IOKit/IOKitLib.h>
/** 
 * @file lsframebuffer-ati.c
 * @brief list framebuffers (active, inactive) of ati (radeon) card(s) - works with 2 cards aswell.
 * @author sfinktah
 */
#define Endian16_Swap(value) \
         ((((UInt16)((value) & 0x00FF)) << 8) | \
                   (((UInt16)((value) & 0xFF00)) >> 8))


typedef u_char byte;

typedef struct {
   int ConnectorType;    /* known values below */
#define CONNECTORTYPE_LVDS      0x00000002    /* Ie internal Low Voltage display, such as laptop */
#define CONNECTORTYPE_DVI_DUAL  0x00000004    /* Dual link(?) DVI */
#define CONNECTORTYPE_VGA       0x00000010    /* Per mucha */
#define CONNECTORTYPE_SVIDEO    0x00000080    /* Per mucha */
#define CONNECTORTYPE_DVI       0x00000200    /* Single link DVI(?), Per azimutz */
#define CONNECTORTYPE_DP        0x00000400    /* Displayport */
#define CONNECTORTYPE_HDMI      0x00000800

   /* Note, could be interesting parallels with Intel HD 3000 Framebuffer:
    * (but there almost definately isn't)
    *
    * (Someone wrote): I hex edited the AppleIntelSNBGraphicsFB binary
    * ( i changed the value from 503 to 602 for VGA port)
    *
    * AppleIntelSNBGraphics binary patched first FrameBuffer table
    *
    * original FB table
    * 0102 0400 1007 0000 1007 0000
    * 0503 0000 0200 0000 3000 0000
    * 0205 0000 0004 0000 0700 0000
    * 0304 0000 0004 0000 0900 0000
    * 0406 0000 0004 0000 0900 0000
    *
    * Patch 0503 -> 0602
    *
    * The four lines should be
    * 0503 - Internal LCD -> 0602 - VGA
    * 0205 - HDMI
    * 0304 - VDI
    * 0406 - HDMI 
    */

   int controlflags;
   int features;
   byte link_i2cid;    /* Bits 0-3: i2cid
                          Bits 4-7: link transmitter link */
   byte dac_digidx;    /* Bits 0-3: link encoder number
                          Bits 4-7: link dac number */
   byte hotplugid;
   byte senseid;    /* Sense line is bits 0-3
                       Use hw i2c flag is bit 4 */
   /* i2cid = (senseid & 0xf-1)+0x90 */
   /* senseid = (i2cid & 0xf) +1*/
} ConnectorInfo;

/*
 * Personality: Uakari
 * ConnectorInfo count in decimal: 4
 * Disk offset in decimal 507640
 * 0000000    00  04  00  00  04  03  00  00  00  01  00  00  12  04  01  05
 * 0000010    00  08  00  00  04  02  00  00  00  01  00  00  11  02  04  03
 * 0000020    10  00  00  00  10  00  00  00  00  01  00  00  00  00  00  02
 *            {ConnectorType} {ControlFlags} {Features     }  link
 *                                                                dac
 *                                                                    hotplug
 *                                                                        senseid
 */
char *get_connector_type(int type_number) {
   switch (type_number) {
      case 0x02:
         return "LVDS";
      case 0x04:
         return "DL-DVI";
      case 0x10:
         return "VGA";
      case 0x80:
         return "SVIDEO";
      case 0x200:
         return "SL-DVI";
      case 0x400:
         return "DP";
      case 0x800:
         return "HDMI";
      default:
         return "????";
   }
}

int main(void)
{
   kern_return_t kr;
   io_iterator_t pciDeviceList;
   io_service_t  pciDevice;
   io_name_t     deviceName;
   io_string_t   devicePath;

   io_struct_inband_t port_number;  
   io_struct_inband_t name;   

   uint32_t name_len;

   io_struct_inband_t compatible;   
   uint32_t compatible_len;

   io_struct_inband_t control_flags;   
   uint32_t control_flags_len;

   io_struct_inband_t device; 
   uint32_t device_len;




   // get an iterator for all PCI devices
   if (IOServiceGetMatchingServices(kIOMasterPortDefault,
            IOServiceMatching("AtiFbStub"),
            // IOServiceMatching("AppleIntelSNBGraphicsFB"),
            &pciDeviceList) != KERN_SUCCESS)
   {
      fprintf(stderr, "Failed IOServiceGetMatchingServices( AtiFbStub )\n");
      return 1;
   }

   printf("Prt Conect CtrFlg Conect Display\n");

   while ((pciDevice = IOIteratorNext(pciDeviceList))) {

      kr = IORegistryEntryGetName(pciDevice, deviceName);
      if (kr != KERN_SUCCESS)
         goto next;

      kr = IORegistryEntryGetPath(pciDevice, kIOServicePlane, devicePath);
      if (kr != KERN_SUCCESS)
         goto next;
      bzero(compatible, 1024);
      bzero(port_number, 1024);
      bzero(device, 1024);
      bzero(name, 1024);

      name_len = 1024;
      device_len = 1024;
      compatible_len = 1024;


      /*
      ATY,ActiveFlags
      ATY,ControlFlags
      av-signal-type
      connector-type
      device_type
      display-connect-flags
      display-type
      port-number
      */

      kr = IORegistryEntryGetProperty(pciDevice, "port-number", port_number, &name_len);
      if (kr != KERN_SUCCESS) {
         printf("Error finding port-number");
         *port_number = 0;
      }

      kr = IORegistryEntryGetProperty(pciDevice, "ATY,ControlFlags", control_flags, &name_len);
      if (kr != KERN_SUCCESS) {
         printf("Error finding ATY,ControlFlags");
         *device = 0;
      }

      kr = IORegistryEntryGetProperty(pciDevice, "connector-type", name, &name_len);
      if (kr != KERN_SUCCESS) {
         printf("Error finding connector-type");
         *name = 0;
      }

      kr = IORegistryEntryGetProperty(pciDevice, "display-type", device, &device_len);
      if (kr != KERN_SUCCESS) {
         printf("Error finding display-type");
         *device = 0;
      }


      int iname = *(int *)name;
      printf("%2d. 0x%04x 0x%04x %-6s %-5s\n", *(int *)port_number, /*Endian16_Swap*/(iname), *(int *)control_flags, get_connector_type(iname), (char *)device);
next:
      IOObjectRelease(pciDevice);
   }

   return kr;
}

/* vim: set ts=3 sts=0 sw=3 noet: */
