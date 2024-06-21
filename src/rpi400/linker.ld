
ENTRY(_start)

SECTIONS
{
    .text 0x80000 : AT(0x0) {
	*(.text) 
	*(.data) 
	*(.rodata.*)
	*(.bss) 
	end = .;
     }
     .shstrtab : {
     	*(.shstrtab)
	}


/*    .data : {
	*(.data) 
	end = .;
     }
*/

     /DISCARD/ : {
	*(.ARM.exidx)
     }
	 
}
