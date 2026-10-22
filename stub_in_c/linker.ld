SECTIONS
{
	. = 0x400000;

    .single_section :
    {
		*(.signature*)
		*(._start*)
        *(.text*)
        *(.rodata*)
    }

	.check_empty : {
		__check_empty_start = .;
		*(.data*)
		*(.bss*)
		__check_empty_end = .;
		__check_empty_size = __check_empty_end - __check_empty_start;
		ASSERT(__check_empty_size == 0, "Error: unexpected section");
	}


	/DISCARD/ :
    {
        *
    }
}
