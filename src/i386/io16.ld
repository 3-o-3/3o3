
ENTRY(_start)

SECTIONS
{
    .text 0x7C00 : AT(0) {
	_start = .;
	*(.text) 
	*(.data)
	*(.bss) 
	end = .;
     }
     .shstrtab : {
     	*(.shstrtab)
	}

     /DISCARD/ : {
	*(.*)
     }
}
