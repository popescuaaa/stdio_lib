#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <windows.h>
#include "so_stdio.h"
#include "utility.h"
#include "constant_values.h"


typedef struct _so_file {
	
	unsigned char buffer_read[BUFFSIZE];
	unsigned char buffer_write[BUFFSIZE];
	int _current_index_read;
	int _current_limit_read;
	int _current_index_write;
	int _read_flag;
	int _write_flag;
	int _append_flag;
	int _dirty_buffer_write;
	int ferror;
	
	BOOL pipe;
	HANDLE handle;
	PROCESS_INFORMATION pi;

}