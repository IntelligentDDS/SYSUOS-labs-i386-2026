TARTGET = test

AS = nasm
CXX = g++

# 编译选项说明：
# - Ubuntu x86_64: sudo apt install gcc-multilib g++-multilib
# - 麒麟OS x86_64: sudo yum install gcc-c++ glibc-devel.i686 libstdc++-devel.i686
# - ARM64 用户: 请参考 assignment.md 中的 ARM64 说明

ASM_FILE = $(wildcard *.asm)
CXX_FILE = $(wildcard *.cpp)

ALL_OBJ += $(ASM_FILE:%.asm=%.o)
ALL_OBJ += $(CXX_FILE:%.cpp=%.o)

run:
	@rm -rf *.o
	@nasm -f elf32 $(ASM_FILE)
	@g++ -m32 -g -c $(CXX_FILE)
	@g++ -m32 -o $(TARTGET) $(ALL_OBJ)
	@./$(TARTGET)
clean:
	@rm -rf *.o