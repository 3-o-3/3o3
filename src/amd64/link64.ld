
ENTRY(_start)

SECTIONS
{
    .text 0x010000 + 1024 : AT(0) {
	_start = .;
	*(.text) 
	*(.data)
    *(.rodata*)
    *(.text.theend)
	*(.bss)
	
    end = .;
     }
     //.shstrtab : {
     	//*(.shstrtab)
    // }
        
    /DISCARD/ :{
        *(.dynsym)
        *(.gnu.hash)
        *(.rel.dyn)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.dynstr)
    }
}
