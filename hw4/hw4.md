# note
* In study of growproc() -> allocuvm() -> mappages() -> walkpgdir()
  * kalloc() itself maintain a free list, each node of list is "free physical memory"
  * When calling mappages(), it is werid at first time see that it cast sz as *va into mappages(), I think it would think like for the process , the user process memory space is start from zero.
  * allocuvm() in loop each time allocate one page from kalloc, then map each newly allocated page (va -> pa)
  * so mappages can be explain like starting from mapping va -> pa, for size

# answer to part 1
* it we skip page allocation and just only increase myproc()->sz, the problem would be that the process itself thought it has the process memory space up to newsz, but when process try to access the memory on that newsz, mmu can't find the va -> pa mapping and return page fault