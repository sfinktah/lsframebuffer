// lsframebuffer.c
//    gcc -mmacosx-version-min=10.5 -Wall -o lsframebuffer lsframebuffer.c -framework IOKit     -framework ApplicationServices

// References: 
// 	http://www.osx86.net/notebooks/18599-success-enable-vga-hdmi-video-intel-hd-3000-a.html
// 	http://www.insanelymac.com/forum/topic/259705-editing-custom-connector-info-for-intel-hd-3000-graphics-sandy-bridge-osx-lion/
//


// Maybe include "AAPL,boot-display"
//
#include <stdio.h>
#include <strings.h>
#include <IOKit/IOKitLib.h>
#define Endian16_Swap(value) \
   ((((UInt16)((value) & 0x00FF)) << 8) | \
    (((UInt16)((value) & 0xFF00)) >> 8))


// http://www.opensource.apple.com/source/IOGraphics/IOGraphics-373.2/IOGraphicsFamily/IOKit/graphics/IOGraphicsTypes.h
// CFNumber/CFData
// #define kIOFBAVSignalTypeKey            "av-signal-type"
// enum {
//     kIOFBAVSignalTypeUnknown = 0x00000000,
//     kIOFBAVSignalTypeVGA     = 0x00000001,
//     kIOFBAVSignalTypeDVI     = 0x00000002,
//     kIOFBAVSignalTypeHDMI    = 0x00000008,
//     kIOFBAVSignalTypeDP      = 0x00000010,
// };
// For Samsung: ambient-light-sensor



typedef u_char byte;
static void *SignalTypeKey[] =
{
   [0x00] = "unkn",
   [0x01] = "VGA",
   [0x02] = "DVI",
   [0x08] = "HDMI",
   [0x10] = "DP",
};


typedef struct {
   int ConnectorType;    /* known values below */
#define CONNECTORTYPE_LVDS      0x00000002    /* Ie internal Low Voltage display, such as laptop */
#define CONNECTORTYPE_DVI_DUAL  0x00000004    /* Dual link(?) DVI */
#define CONNECTORTYPE_VGA       0x00000010    /* Per mucha */
#define CONNECTORTYPE_SVIDEO    0x00000080    /* Per mucha */
#define CONNECTORTYPE_DVI       0x00000200    /* Single link DVI(?), Per azimutz */
#define CONNECTORTYPE_DP        0x00000400    /* Displayport */
#define CONNECTORTYPE_HDMI      0x00000800

   /* Intel HD 3000 Framebuffer:
   */
   // /* 12 byte connectorinfo */
   // 
   // /* 60 byte total */
   // typedef struct {
   //         char byte0; /* Presence? */
   //         char byte1; /* For display pipe max connector index? */
   //         char byte2; /* Usable Connector count */
   //         char byte3; /* Appears unused */
   //         char byte[8];
   //         connectorinfo_t connectors[4];
   // } PlatformInformationList_type;
   // 
   // PlatformInformationList_type PlatformInformationList[8];
   /*
    *
    * I hex edited the AppleIntelSNBGraphicsFB binary
    * ( i changed the value from 503 to 602 for VGA port)
    *
    * AppleIntelSNBGraphics binary patched first FrameBuffer table
    *
   // typedef struct {
   //            char byte0;
   //            char byte1; // i2cid? 
   //            char unused[2];
   //            uint32          connectortype;       // The connector type, see below 
   //            char byte[4];
   // } connectorinfo_t;
    *
    * #define CONNECTORTYPE_LVDS	0x00000002	
    * #define CONNECTORTYPE_DP	0x00000400
    * #define CONNECTORTYPE_HDMI	0x00000800

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
 *
 * Personality: Uakari
 * ConnectorInfo count in decimal: 4
 * Disk offset in decimal 507640
 * 0000000    00  04  00  00  04  03  00  00  00  01  00  00  12  04  01  05
 * 0000010    00  08  00  00  04  02  00  00  00  01  00  00  11  02  04  03
 * 0000020    10  00  00  00  10  00  00  00  00  01  00  00  00  00  00  02
 *				  {ConnectorType} {ControlFlags} {Features     }  link
 *				  																	   dac
 *				  																	   	 hotplug
 *				  																	   	 	  senseid
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

void * getIoReg(char *target, char *key) {
   kern_return_t kr;
   io_iterator_t pciDeviceList;
   io_service_t  pciDevice;
   io_name_t     deviceName;
   io_string_t   devicePath;
   void * rv = NULL;

   if (IOServiceGetMatchingServices(kIOMasterPortDefault,
	    IOServiceMatching(target), 
	    &pciDeviceList) != KERN_SUCCESS)
   {
      fprintf(stderr, "Failed IOServiceGetMatchingServices( '%s' )\n", target);
      return rv;
   }


   // fprintf(stderr, "Iterating...\n");
   while ((pciDevice = IOIteratorNext(pciDeviceList))) {
      // fprintf(stderr, "IOIteratorNext...\n");
      /*
       * kern_return_t
       * IORegistryEntryGetName(
       * 	io_registry_entry_t	entry,
       * 		io_name_t 	        name );
       */


      kr = IORegistryEntryGetName(pciDevice, deviceName);
      if (kr != KERN_SUCCESS)
	 goto next;

      kr = IORegistryEntryGetPath(pciDevice, kIOServicePlane, devicePath);
      if (kr != KERN_SUCCESS)
	 goto next;

      /*
	 kern_return_t
	 IORegistryEntryGetProperty(
	 io_registry_entry_t	entry,
	 const io_name_t		propertyName,
	 io_struct_inband_t	buffer,
	 uint32_t	      * size );
	 */

      static io_struct_inband_t value;
      uint32_t	value_len = 1024;
      // bzero(value, 1024); // shouldn't that be 4096 anyway	io_struct_inband_t[4096];

      kr = IORegistryEntryGetProperty(pciDevice, key, value, &value_len);
      if (kr != KERN_SUCCESS) {
	 fprintf(stderr, "Error finding key '%s'", key);
      }
      // printf("%2d. 0x%04x 0x%04x %-4s %-6s\n", *(int *)port_number, /*Endian16_Swap*/(iname), *(int *)control_flags, (char *)SignalTypeKey[ *(int *)device], get_connector_type(iname));
      rv = (void *)value;

next:
      IOObjectRelease(pciDevice);
   }

   return rv;
}

   int
main(void)
{
   void *value = getIoReg("AppleIntelSNBGraphicsFB", "NumFrameBuffer");
   if (!value) {
      printf("AppleIntelSNBGraphicsFB not found\n");
   } else {
      printf("AppleIntelSNBGraphicsFB :: NumFrameBuffer = %d\n", *(int *)value);
   }
   // printf("%2d. 0x%04x 0x%04x %-4s %-6s\n", *(int *)port_number, /*Endian16_Swap*/(iname), *(int *)control_flags, (char *)SignalTypeKey[ *(int *)device], get_connector_type(iname));



   kern_return_t kr;
   io_iterator_t pciDeviceList, childList;
   io_object_t 	childObj;
   io_service_t  pciDevice;
   io_name_t     deviceName;
   io_string_t   devicePath;

   io_struct_inband_t name;	// typedef char 			io_struct_inband_t[4096];
   uint32_t	name_len;

   io_struct_inband_t compatible;	// typedef char 			io_struct_inband_t[4096];
   uint32_t	compatible_len;

   // io_struct_inband_t control_flags;	// typedef char 			io_struct_inband_t[4096];
   // uint32_t	control_flags_len;

   io_struct_inband_t device;	// typedef char 			io_struct_inband_t[4096];
   uint32_t	device_len;

   io_struct_inband_t subsystem_device;	// typedef char 			io_struct_inband_t[4096];
   uint32_t	subsystem_device_len;

   io_struct_inband_t subsystem_vendor;	// typedef char 			io_struct_inband_t[4096];
   uint32_t	subsystem_vendor_len;


   //
   // get an iterator for all PCI devices
   if (IOServiceGetMatchingServices(kIOMasterPortDefault,
	    // IOServiceMatching("AtiFbStub"),
	    IOServiceMatching("AppleIntelFramebuffer"),
	    &pciDeviceList) != KERN_SUCCESS)
   {
      fprintf(stderr, "Failed IOServiceGetMatchingServices( AppleIntelFramebuffer )\n");
      return 1;
   }

   printf("        Port# Conect Accel# SigTyp Conect[Full]\n");

   // fprintf(stderr, "Iterating...\n");
   while ((pciDevice = IOIteratorNext(pciDeviceList))) {
      // fprintf(stderr, "IOIteratorNext...\n");
      /*
       * kern_return_t
       * IORegistryEntryGetName(
       * 	io_registry_entry_t	entry,
       * 		io_name_t 	        name );
       */


      kr = IORegistryEntryGetName(pciDevice, deviceName);
      if (kr != KERN_SUCCESS)
	 goto next;

      kr = IORegistryEntryGetPath(pciDevice, kIOServicePlane, devicePath);
      if (kr != KERN_SUCCESS)
	 goto next;

      /*
	 kern_return_t
	 IORegistryEntryGetProperty(
	 io_registry_entry_t	entry,
	 const io_name_t		propertyName,
	 io_struct_inband_t	buffer,
	 uint32_t	      * size );
	 */

      // bzero(control_flags, 1024);
      bzero(compatible, 1024);
      bzero(device, 1024);
      bzero(name, 1024);

      name_len = 1024;
      device_len = 1024;
      subsystem_device_len = 1024;
      subsystem_vendor_len = 1024;
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

      /*
	 | |   | +-o AppleIntelSNBGraphicsFB  <class AppleIntelSNBGraphicsFB,
	 id 0x10000022b,
	 registered,
	 matched,
	 active,
	 busy 0 (28 ms),
	 retain 9>
	 | |   | | | {
	 | |   | | |   "IOClass" = "AppleIntelSNBGraphicsFB"
	 | |   | | |   "CFBundleIdentifier" = "com.apple.driver.AppleIntelSNBGraphicsFB"
	 | |   | | |   "FeatureControl" = {"Watermarks"=1,"RenderStandby"=1,"GPUInterruptHandling"=1,"DPM"=1,"MaximumSelfRefreshLevel"=3,"Gamma"=1,"PowerStates"=1,"IgnorePanelTimings"=0,"UseInterruptFilter"=1,"SpreadON"=1,"CachedEDIDDisable"=0,"FastDisplayDetectDisable"=0,"FBC"=0,"SetRC6Voltage"=0}
	 | |   | | |   "IOProviderClass" = "IOPCIDevice"
	 | |   | | |   "IOPCIClassMatch" = "0x03000000&0xff000000"
	 | |   | | |   "IOSourceVersion" = "8.10.44"
	 | |   | | |   "FBCControl" = {"Compression"=0}
	 | |   | | |   "IOProbeScore" = 80000
	 | |   | | |   "NumFrameBuffer" = <03> 
	 | |   | | |   "VRAM,totalsize" = <00000020>
	 | |   | | |   "IOMatchCategory" = "IOFramebuffer"
	 | |   | | |   "dpm" = <01000000>
	 | |   | | |   "fRC6HandlingFuncs" = <b801911280ffffff>
	 | |   | | |   "IOPCIPrimaryMatch" = "0x01068086 0x11068086 0x16018086 01168086 01268086"
	 | |   | | | } 
	 */

      /*
	 | |   | +-o AppleIntelFramebuffer@0  <class AppleIntelFramebuffer, id 0x100000229, registered, matched, active, busy 0 (3 ms), retain 13>
	 | |   | | | {
	 | |   | | |   "IOVARendererID" = 0x1081000
	 | |   | | |   "IOFBGammaCount" = 0x400
	 | |   | | |   "IOFBConfig" = {"IOFB0Hz"=Yes,"IOFBModes"=({"ID"=0xffffffffffffff00,"DM"=<010000000100000000000000000000000700000000000000000000000000000000000000>,"AID"=0x226})}
	 | |   | | |   "IODVDBundleName" = "AppleIntelSNBVA"
	 | |   | | |   "IOPMStrictTreeOrder" = Yes
	 | |   | | |   "IOI2CTransactionTypes" = 0x1f
	 | |   | | |   "IOFBGammaWidth" = 0xa
	 | |   | | |   "IOPowerManagement" = {"MaxPowerState"=0x2,"CurrentPowerState"=0x2,"ChildProxyPowerState"=0x2,"DriverPowerState"=0x1}
	 | |   | | |   "AAPL,DisplayPipe" = <ffff0000>
	 | |   | | |   "IOFramebufferOpenGLIndex" = 0x0
	 | |   | | |   "connector-type" = <02000000>
	 | |   | | |   "IOFBI2CInterfaceIDs" = (0xa52800000000000)
	 | |   | | |   "IOFBCursorInfo" = ()
	 | |   | | |   "IOFBWaitCursorFrames" = 0x17
	 | |   | | |   "IOFBDependentID" = 0xffffff800a73a000
	 | |   | | |   "IOFBUIScale" = <00000000>
	 | |   | | |   "IOAccelRevision" = 0x2
	 | |   | | |   "IOFBNeedsRefresh" = Yes
	 | |   | | |   "IOCFPlugInTypes" = {"ACCF0000-0000-0000-0000-000a2789904e"="AppleIntelHD3000GraphicsGA.plugin"}
	 | |   | | |   "IOFBCLUTDefer" = Yes
	 | |   | | |   "IOFBTimingRange" = <0000000000000000000000000000000000000000000000000000000000000000002d31010000000080df171000000000000000000f0000000f0000000800000000000000ffffffff00000000ffffffffff1f0000ff1f0000000000000000000001010101010101010101010101010000000000000020000000000000ff1f000000000000ff1f000000000000ff1f000000000000000c000000000000ff1f000000000000ff1f000000000000ff1f000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000>
	 | |   | | |   "IOAccelIndex" = 0x0
	 | |   | | |   "IOFBTransform" = 0x0
	 | |   | | |   "IOFBDependentIndex" = 0x0
	 | |   | | |   "IOFBI2CInterfaceInfo" = ({"IOI2CInterfaceID"=0x0,"IOI2CTransactionTypes"=0x1f})
	 | |   | | |   "IODisplayParameters" = "IOFramebufferParameterHandler is not serializable"
	 | |   | | |   "port-number" = 0x0
	 | |   | | |   "IOGeneralInterest" = "IOCommand is not serializable"
	 | |   | | |   "IOFBScalerInfo" = <000000000000000000000000000000000a000000ff0f0000ff0f00000000000000000000000000000000000000000000>
	 | |   | | |   "IOFBWaitCursorPeriod" = 0x1fca055
	 | |   | | |   "IOFBProbeOptions" = 0x401
	 | |   | | |   "IOAccelTypes" = "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/IGD0@2/Gen6Accelerator"
	 | |   | | | }
	 */
      int iname = 1;
      io_struct_inband_t port_number;	// typedef char 			io_struct_inband_t[4096];
      bzero(port_number, 1024);
      kr = IORegistryEntryGetProperty(pciDevice, "port-number", port_number, &name_len);
      if (kr != KERN_SUCCESS) {
	 printf("Error finding port-number");
	 *port_number = 0;
      }


      io_struct_inband_t control_flags;	// typedef char 			io_struct_inband_t[4096];
      kr = IORegistryEntryGetProperty(pciDevice, "IOAccelIndex", control_flags, &name_len);
      if (kr != KERN_SUCCESS) {
	 printf("Error finding IOAccelIndex");
	 *control_flags = 0;
      }


      kr = IORegistryEntryGetProperty(pciDevice, "connector-type", name, &name_len);
      if (kr != KERN_SUCCESS) {
	 printf("Error finding connector-type");
	 iname = 0;
      }

      // port-number
      // av-signal-type

      kr = IORegistryEntryGetProperty(pciDevice, "av-signal-type", device, &device_len);
      if (kr != KERN_SUCCESS) {
	 printf("          ");
	 *device = 0;
      } else {
	 printf("Connected ");
      }

      // av-signal-type: value must be 0x8 for HDMI or 0x10 for displayport

      if (iname) {
	 iname = *(int *)name;
      }

      printf("%2d. 0x%04x 0x%04x %-4s %-6s\n", *(int *)port_number, /*Endian16_Swap*/(iname), *(int *)control_flags, (char *)SignalTypeKey[ *(int *)device], get_connector_type(iname));

      /*

	 kr = IORegistryEntryGetChildIterator(
	 pciDevice, kIOServicePlane, &childList);

	 if (kr != KERN_SUCCESS) {
	 printf("Error chasing children\n");
	 }

	 if ((childObj = IOIteratorNext(childList)) == 0) {
      // printf("0 child objects - unsupported device\n");
      }

      int iDevice = *(int *)device;

      printf("%s %04x:%04x %s %s\n", 
      childObj ? "Driver\t" : "      \t",
       *iVendor, iDevice, names_vendor(*iVendor), names_product(*iVendor, iDevice)
       );

       printf("%s %04x:%04x [%04x:%04x] %s %s\n", 
       childObj ? "Driver\t" : "      \t",
       0, 0,
       *(int *)subsystem_vendor, 
       *(int *)subsystem_device,

       "", ""
       );

      // don't print the plane name prefix in the device path

      printf("%12s: (%04x:%04x) %s\n\t%s (%s)\n", 
      name, *iVendor, *(int *)(device), 
      childObj ? "" : "*NO DRIVER FOR THIS DEVICE*\n",
      &devicePath[9], deviceName);
      // printf("%12s: (%4s:%4s) %s (%s)\n", name, vendor, device, &devicePath[9], deviceName);
      */

next:
      IOObjectRelease(pciDevice);
   }

   return kr;
}


/* vim: set ts=8 sts=0 sw=3 noet: */
