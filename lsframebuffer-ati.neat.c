/** 
 * @file lsframebuffer-ati.c
 * @brief List ATI Framebuffer information from IORegistry
 * @author Christopher Anderson
 * @date 2011-10-15
 * @license MIT (do whatever the hell you want with it)
 */
//  gcc -mmacosx-version-min=10.5 -Wall -o lsframebuffer lsframebuffer.c -framework IOKit     -framework ApplicationServices

#include <stdio.h>
#include <strings.h>
#include <IOKit/IOKitLib.h>
#define Endian16_Swap(value) ((((UInt16)((value) & 0x00FF)) << 8) | (((UInt16)((value) & 0xFF00)) >> 8))

typedef u_char byte;
char *get_connector_type(int type_number) {
    switch (type_number) {
        case 0x02: return "LVDS";
        case 0x04: return "DL-DVI";
        case 0x10: return "VGA";
        case 0x80: return "SVIDEO";
        case 0x200: return "SL-DVI";
        case 0x400: return "DP";
        case 0x800: return "HDMI";
        default: return "????";
    }
}

int main(void)
{
    kern_return_t kr;
    io_iterator_t pciDeviceList;
    io_service_t  pciDevice;
    io_name_t     deviceName;
    io_string_t   devicePath;

    io_struct_inband_t name;    
    io_struct_inband_t compatible;  
    io_struct_inband_t control_flags;   
    io_struct_inband_t device;
    uint32_t    name_len;
    uint32_t    compatible_len;
    uint32_t    device_len;


    // get an iterator for all PCI devices
    if (IOServiceGetMatchingServices(kIOMasterPortDefault,
                IOServiceMatching("AtiFbStub"),
                &pciDeviceList) != KERN_SUCCESS)
    {
        fprintf(stderr, "Failed IOServiceGetMatchingServices( AtiFbStub )\n");
        return 1;
    }


    while ((pciDevice = IOIteratorNext(pciDeviceList))) {

        kr = IORegistryEntryGetName(pciDevice, deviceName);
        if (kr != KERN_SUCCESS)
            goto next;

        kr = IORegistryEntryGetPath(pciDevice, kIOServicePlane, devicePath);
        if (kr != KERN_SUCCESS)
            goto next;

        bzero(control_flags, 1024); bzero(compatible, 1024); bzero(device, 1024); bzero(name, 1024); 
        name_len = 1024; device_len = 1024; compatible_len = 1024;

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

        kr = IORegistryEntryGetProperty(pciDevice, "ATY,ControlFlags", control_flags, &name_len);
        if (kr != KERN_SUCCESS) {
            printf("Error finding ATY,ControlFlags");
            *device = 0;
        }

        int iname = *(int *)name;
        printf("0x%04x %04x %-6s %-5s\n", Endian16_Swap(iname), *(int *)control_flags, get_connector_type(iname), (char *)device);


next:
        IOObjectRelease(pciDevice);
    }

    return kr;
}


