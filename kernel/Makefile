TARGET = kernel.elf
OBJS = main.o

INCLUDE_DIR += /usr/lib/llvm-10/
EDK2DIR			+= $HOME/develop/mikanos/edk2

CXXFLAGS 	+=	-O2 --target=x86_64-elf -fno-exceptions -ffreestanding -mno-red-zone -fno-rtti -std=c++17
INCLUDES  += 	-I$(INCLUDE_DIR)/include/c++/v1 -I$(INCLUDE_DIR)/include -I$(INCLUDE_DIR)/include/freetype2 \
							-I$(EDK2DIR)/MdePkg/Include -I$(EDK2DIR)/MdePkg/Include/X64 \
							-nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
							-DEFIAPI='__attribute__((ms_abi))'
LDFLAGS 	+=	--entry KernelMain -z norelro --image-base 0x100000 --static

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
				rm -rf *.o

kernel.elf: $(OBJS) Makefile
				ld.lld $(LDFLAGS) -o kernel.elf $(OBJS)

%.o: %.cpp Makefile
				clang++ $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $<
