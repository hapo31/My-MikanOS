TARGET = kernel.elf
OBJS = main.o font.o graphics.o hankaku.o console.o asmfunc.o pci.o logger.o mouse_.o \
			interrupt.o memory_manager.o paging.o segment.o window.o layer.o \
			libcxx_support.o newlib_support.o \
			usb/memory.o usb/device.o usb/xhci/ring.o usb/xhci/trb.o usb/xhci/xhci.o \
			usb/xhci/port.o usb/xhci/device.o usb/xhci/devmgr.o usb/xhci/registers.o \
			usb/classdriver/base.o usb/classdriver/hid.o usb/classdriver/keyboard.o \
			usb/classdriver/mouse.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=d))))

INCLUDE_DIR += $(HOME)/develop/mikanos/devenv/x86_64-elf
EDK2DIR			+= $(HOME)/develop/mikanos/edk2

CXXFLAGS 	+=	-O2 --target=x86_64-elf -fno-exceptions -ffreestanding -mno-red-zone -fno-rtti -std=c++17
INCLUDES  += 	-I$(INCLUDE_DIR)/include/c++/v1 -I$(INCLUDE_DIR)/include -I$(INCLUDE_DIR)/include/freetype2 \
							-I/usr/lib/llvm-10/include \
							-I$(EDK2DIR)/MdePkg/Include -I$(EDK2DIR)/MdePkg/Include/X64 \
							-I./ \
							-nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
							-DEFIAPI='__attribute__((ms_abi))'

LIBS			+= 	-lc -lc++ -lc++experimental -lc++abi -lg -lm -lfreetype

LDFLAGS 	+=	--entry KernelMain -z norelro --image-base 0x100000 --static \
							-L$(INCLUDE_DIR)/lib -L/usr/lib/llvm-10/lib $(LIBS)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
				rm -rf *.o
				rm -rf ./usb/*.o
				rm -rf ./usb/classdriver/*.o
				rm -rf ./usb/xhci/*.o
kernel.elf: $(OBJS) mouse.o Makefile
				ld.lld $(LDFLAGS) -o kernel.elf $(OBJS)

%.o: %.cpp Makefile
				clang++ $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $<
				mv $(notdir $*.o) $(dir $<) || true

mouse_.o: mouse.cpp Makefile
				clang++ $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c mouse.cpp -o mouse_.o

.%.d: %.cpp
				clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
				$(eval OBJ = $(<:.cpp=.o))
				sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@


.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)
