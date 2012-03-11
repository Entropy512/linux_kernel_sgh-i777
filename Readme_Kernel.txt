HOW TO BUILD KERNEL 2.6.35 FOR GT-I9100P

1. How to Build
    - get Toolchain
        From Codesourcery site( http://www.codesourcery.com )
        recommand :
          Feature : ARM
          Target OS : "EABI"
          package : "IA32 GNU/Linux TAR"

      Ex) Download : http://www.codesourcery.com/sgpp/lite/arm/portal/package7813/public/arm-none-eabi/arm-2010.09-51-arm-none-eabi-i686-pc-linux-gnu.tar.bz2

    - edit Makefile
      edit "CROSS_COMPILE" to right toolchain path(You downloaded).
       Ex) ARCH		?= arm
           CROSS_COMPILE	= ./arm-2010.09/bin/arm-none-eabi-

   - command
      $ make c1_rev02_nfc_defconfig
      $ make

2. Output files : ./arch/arm/boot/zImage
	
3. How to make .tar binary for downloading into target.
	- change current directory to kernel/arch/arm/boot
	- type following command
	$ tar cvf GT-I9100P_Kernel_Gingerbread.tar zImage
	
