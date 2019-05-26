# note
* https://pdos.csail.mit.edu/6.828/2018/lec/l-josmem.html
* https://github.com/SimpCosm/6.828/tree/master/lab2
* https://pdos.csail.mit.edu/6.828/2018/labs/lab2/
# questions
* GDB to check the address of end[]
* I was confused why in page_init, set pages between [basemem, basemem + num_kpages] to all used, it is due to as code itself said, num_kpages is allocated from last addr of kernel text code, which means those memory are occupied. So that [basemem, basemem + num_kpages] area are fully used
* page_remove name is a bit confusing, actaully it is to remove va mapping, so reduce one count on the page
* I was having issue in page_insert(), it turns out due to I should increase pp->pp_ref before caling page_remove, otherwise, if I increase count after remove , the page might be freed though the page count still be in due to I increment after remove.
* https://github.com/SimpCosm/6.828/tree/master/lab2