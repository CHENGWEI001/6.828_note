# lab1
* exercise 3
	```
	Below assembly code is for loop for below for loop in bootmain()
		for (; ph < eph; ph++)
			// p_pa is the load address of this segment (as well
			// as the physical address)
			readseg(ph->p_pa, ph->p_memsz, ph->p_offset);

	   0x7d51:      cmp    %esi,%ebx
	   0x7d53:      jae    0x7d6b
	   0x7d55:      pushl  0x4(%ebx)
	   0x7d58:      pushl  0x14(%ebx)
	   0x7d5b:      add    $0x20,%ebx
	   0x7d5e:      pushl  -0x14(%ebx)
	   0x7d61:      call   0x7cdc
	   0x7d66:      add    $0xc,%esp
	   0x7d69:      jmp    0x7d51

	  the start of the loop go to 0x7d51 , check if ph equal to eph, if equal which would fail ph < eph, then it jump out of loop by doing "0x7d53:      jae    0x7d6b", and each round , it is increasing ph which maps to "0x7d5b:      add    $0x20,%ebx" in assembly code, 0x20 is 32 bytes which is the size of below structure
	  struct Proghdr {
		uint32_t p_type;
		uint32_t p_offset;
		uint32_t p_va;
		uint32_t p_pa;
		uint32_t p_filesz;
		uint32_t p_memsz;
		uint32_t p_flags;
		uint32_t p_align;
	};
	```
  * At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?  
    A: I think .code16 and .code32 is the point to switch 
  * What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
    A: probably just look at bootloader and kernel assembly, easy to find out    
    last botloader inst: ```=> 0x7d6b:      call   *0x10018```
    first kernel assembly: ```=> 0x10000c:    movw   $0x1234,0x472```
  * Where is the first instruction of the kernel?  
    A: can be found in kernel.asm  
  * How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?  
    A: elf has the size , and readSect read the size per sect which is 512bytes
