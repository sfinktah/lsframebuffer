### lsframebuffer-ati
Displays current ATI/AMD/Radeon framebuffer

### lsframebuffer-ati.neat
(the same as the above, but with much tidier source code with lots of crud removed)

### lsframebuffer-intel
Displays current Intel framebuffer

### lsframebuffer-nvidia
Sorry, never had a NVidia when I wrote these, so there's no such thing.  But I'm sure it would be easy to make.

### sample output
```
admin@mac $ ./lsframebuffer-ati
Prt Conect CtrFlg Conect Display
 3. 0x0004 0x0004 DL-DVI LCD
 4. 0x0004 0x0014 DL-DVI LCD
 0. 0x0400 0x0304 DP     NONE
 1. 0x0400 0x0304 DP     NONE
 2. 0x0800 0x0204 HDMI   NONE
```

### how it works
I forget, but it's really just manipulation of **ioreg** output, nothing fancy.  It's just a useful tool to quickly know if your framebuffers are the way you want 'em.

### Binaries

(included)

### Source

(included, because you should never run strange binaries)


### Compiling

```
gcc -mmacosx-version-min=10.5 -Wall -o lsframebuffer lsframebuffer.c -framework IOKit -framework ApplicationServices
```

Although that *10.5* thing probably will probably send your 10.11.x compiler into conniptions, so YMMV.  Personally, I'm still using 10.8.5 because that's how I roll.

FTR, binaries compiled with the above options still run perfectly on 10.11.x.


### Related tools and links

[redsock bios decoder](http://nologic.com/redsock_bios_decoder.zip)
[Easy IOReg Extraction (DSDT, SSDT, Video BIOS, ...)](https://www.tonymacx86.com/threads/easy-ioreg-extraction-dsdt-ssdt-video-bios.81174/)
[Getting all available framebuffers](https://www.tonymacx86.com/threads/apple-intel-amd-ati-framebuffers.112299/)
[AMD Graphics Guide | Rampage Dev](http://www.rampagedev.com/?page_id=82)




To quickly grab a copy of your VBIOS from within OS X, 
