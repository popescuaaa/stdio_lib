#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <windows.h>
#define DLL_IMPORTS
#include "so_stdio.h"

#define BUFFSIZE                                4096

/****************** File opening modes ********************/
#define READ_ONLY_MODE                          1
#define READ_WRITE_MODE                         2
#define WRITE_CREATE_TRUNCATE_MODE              3
#define READ_WRITE_CREATE_TRUNCATE_MODE         4
#define WRITE_ONLY_CREATE_APPEND_MODE           5
#define READ_WRITE_CREATE_APPEND_MODE           6
/************************* Errors ************************/
#define BAD_ALLOC                               12
#define SO_EOF                                  (-1)
/************* Flags *************/
#define ENABLE									1
#define DISABLE									0
/******************* Return values *********************/
#define SUCCESS                                 0
#define FAIL									(-1)
/******************** Error flags **********************/
#define NON_ERROR                               0
#define WITH_ERROR                              1
/******************* Dirty status *********************/
#define DIRTY									1
#define CLEAN									0

/* There is a flag for the third flag */
/* of access, an append one for knowing */
/* when to reset the handle postion */
typedef struct _so_file {
	HANDLE _file_handle;
	unsigned char _buffer_read[BUFFSIZE];
	unsigned char _buffer_write[BUFFSIZE];
	int _current_index_read;
	int _current_limit_read;
	int _current_index_write;
	int _dirty_buffer_write;
	int _read_flag;
	int _write_flag;
	int _reset_position_flag;
	int _error_flag;
	BOOL _is_pipe;
	PROCESS_INFORMATION _process_info;
} SO_FILE;


static VOID CloseProcess(LPPROCESS_INFORMATION lppi)
{
	CloseHandle(lppi->hThread);
	CloseHandle(lppi->hProcess);
}

static VOID RedirectHandle(STARTUPINFO *psi, HANDLE hFile, INT opt)
{
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	ZeroMemory(psi, sizeof(STARTUPINFO));
	psi->cb = sizeof(STARTUPINFO);
	psi->hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	psi->hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	psi->hStdError = GetStdHandle(STD_ERROR_HANDLE);
	psi->dwFlags |= STARTF_USESTDHANDLES;
	/* Prelevated from the lab */
	switch (opt) {
	case STD_INPUT_HANDLE:	
		psi->hStdInput = hFile;
		break;
	case STD_OUTPUT_HANDLE:
		psi->hStdOutput = hFile;
		break;
	case STD_ERROR_HANDLE:
		psi->hStdError = hFile;
		break;
	}
}

int open_mode(const char *mode)
{
    if (strcmp(mode, "r") == 0) 
		return READ_ONLY_MODE;
    if (strcmp(mode, "r+") == 0) 
		return READ_WRITE_MODE;
    if (strcmp(mode, "w") == 0) 
		return WRITE_CREATE_TRUNCATE_MODE;
    if (strcmp(mode, "w+") == 0) 
		return READ_WRITE_CREATE_TRUNCATE_MODE;
    if (strcmp(mode, "a") == 0) 
		return WRITE_ONLY_CREATE_APPEND_MODE;
    if (strcmp(mode, "a+") == 0) 
		return READ_WRITE_CREATE_APPEND_MODE;
    return 0;
}

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	HANDLE _file_handle = INVALID_HANDLE_VALUE;
	int _read_flag;
	int _write_flag;
	int _reset_position_flag;
	int opening_mode = open_mode(mode);
	SO_FILE *new_file;
	
	switch (opening_mode) {
	case READ_ONLY_MODE:
		_file_handle = CreateFile(
			pathname,
			GENERIC_READ,	
			FILE_SHARE_READ, 
			NULL,		  
			OPEN_EXISTING,	  
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		_read_flag = ENABLE;
		_write_flag = DISABLE;
		_reset_position_flag = DISABLE;
		break;
	case READ_WRITE_MODE:
		_file_handle = CreateFile(
			pathname,
			GENERIC_READ | GENERIC_WRITE,	  
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL,		  
			OPEN_EXISTING,	  
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		_read_flag = ENABLE;
		_write_flag = DISABLE;
		_reset_position_flag = DISABLE;
		break;
	case WRITE_CREATE_TRUNCATE_MODE:
		_file_handle = CreateFile(
			pathname,
			GENERIC_WRITE,	  
		    FILE_SHARE_WRITE, 
			NULL,		   
			CREATE_ALWAYS,	   
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		_read_flag = DISABLE;
		_write_flag = ENABLE;
		_reset_position_flag = DISABLE;
		break;
	case READ_WRITE_CREATE_TRUNCATE_MODE:
		_file_handle = CreateFile(
			pathname,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,		 
			CREATE_ALWAYS,	  
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		_read_flag = ENABLE;
		_write_flag = ENABLE;
		_reset_position_flag = DISABLE;
		break;
	case WRITE_ONLY_CREATE_APPEND_MODE:
		_file_handle = CreateFile(
			pathname,
			GENERIC_READ|GENERIC_WRITE,	
			FILE_SHARE_WRITE,
			NULL,		   
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		SetFilePointer(_file_handle, 0, NULL, FILE_END);
		_read_flag = DISABLE;
		_write_flag = ENABLE;
		_reset_position_flag = ENABLE;
		break;
	case READ_WRITE_CREATE_APPEND_MODE:
		_file_handle = CreateFile(
			pathname,
			GENERIC_READ | GENERIC_WRITE,	
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,		  
			OPEN_ALWAYS,	 
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		SetFilePointer(_file_handle, 0, NULL, FILE_END);
		_read_flag = ENABLE;
		_write_flag = ENABLE;
		_reset_position_flag = ENABLE;
		break;
	default:
		break;
	}

	if (_file_handle == INVALID_HANDLE_VALUE)
		return NULL;

	new_file = (SO_FILE *) malloc(sizeof(SO_FILE));

	if (new_file == NULL)
		exit(BAD_ALLOC);

	new_file->_file_handle = _file_handle;
	new_file->_current_index_read = BUFFSIZE;
	new_file->_current_limit_read = BUFFSIZE - 1;
	new_file->_current_index_write = 0;
	new_file->_dirty_buffer_write = CLEAN;
	new_file->_error_flag = NON_ERROR;
	new_file->_read_flag = _read_flag;
	new_file->_write_flag = _write_flag;
	new_file->_reset_position_flag = _reset_position_flag;
	/* Basically we start from the presumption */
	/* that the file is not a pipe yet */
	new_file->_is_pipe = FALSE;
	return new_file;
}

int so_fclose(SO_FILE *stream)
{
	int close_return_value;
	int fflush_op_value;
	
	if (stream->_dirty_buffer_write == DIRTY) {
		if (stream->_reset_position_flag == ENABLE)
			SetFilePointer(stream->_file_handle, 0, NULL, FILE_END);
		fflush_op_value = so_fflush(stream);
		if (fflush_op_value != SUCCESS) {
			stream->_error_flag = WITH_ERROR;
			free(stream);
			return SO_EOF;
		}
	}
	close_return_value = CloseHandle(stream->_file_handle);
	if (close_return_value) {
		free(stream);
		return SUCCESS;
	}
	return FAIL;
}

HANDLE so_fileno(SO_FILE *stream)
{
	return stream->_file_handle;
}

int so_fflush(SO_FILE *stream)
{
	unsigned char *write_data = stream->_buffer_write;
	int write_op_amount;
	BOOL write_op_return;
	
	write_op_return =  WriteFile(
		stream->_file_handle,
		write_data,
		stream->_current_index_write,
		&write_op_amount,
		NULL
	);
	if (write_op_amount <= 0) {
		stream->_error_flag = WITH_ERROR;
		return SO_EOF;
	}
	stream->_current_index_write = 0;
	stream->_dirty_buffer_write = CLEAN;
	return SUCCESS;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int set_file_op_result;
	if (stream->_read_flag == ENABLE)
		/* enable re-reading */
		stream->_current_index_read = BUFFSIZE; 
	if (stream->_write_flag == ENABLE &&
		stream->_current_index_write != 0) {
		so_fflush(stream);
		stream->_current_index_write = 0;
	}
	set_file_op_result = SetFilePointer(
		   stream->_file_handle,
		   offset,
		   NULL,
		   whence
	);
	if (set_file_op_result < 0) {
		stream->_error_flag = WITH_ERROR;
		return FAIL;
	}
	return SUCCESS;
}

long so_ftell(SO_FILE *stream)
{
	int write_amount;
	int read_amount;
	const int default_value = 0;
	int current_pos_file;
	
	if (stream->_read_flag == ENABLE)
		read_amount = stream->_current_limit_read + 1
			+ stream->_current_index_read;
	else
		read_amount = default_value;
	if (stream->_write_flag == ENABLE)
		write_amount = stream->_current_index_write;
	else
		write_amount = default_value;
	current_pos_file = SetFilePointer(
		   stream->_file_handle,
		   0,         
		   NULL, 
		   FILE_CURRENT
	);
	if (current_pos_file <= 0)
		return FAIL;
	return current_pos_file +
		write_amount + read_amount;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int read_amount = 0;
	int current_char;
	int index;
	int total_amount = nmemb * size;
	
	for (index = 0; index < total_amount; index++) {
		current_char = so_fgetc(stream);
		if (current_char == SO_EOF) 
			return read_amount / size;
		*((unsigned char *)ptr + index) = (unsigned char)current_char;
		read_amount++;
	}
	return read_amount / size;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int write_amount = 0;
	int current_char;
	int index;
	int total_amount = nmemb * size;
	
	for (index = 0; index < total_amount; index++) {
		current_char = so_fputc(*((unsigned char *)ptr + index),
			stream);
		if (current_char == SO_EOF)
			return write_amount / size;
		write_amount++;
	}
	return write_amount /  size;
}

int so_fgetc(SO_FILE *stream)
{
	BOOL read_op_return;
	int read_op_status;
	int return_char;
	
	if (stream->_current_index_read > stream->_current_limit_read) {
		read_op_return = ReadFile(
		   stream->_file_handle,
		   stream->_buffer_read,
		   BUFFSIZE,
		   &read_op_status,
		   NULL
		);
		stream->_current_index_read = 0;
		stream->_current_limit_read = read_op_status - 1;
		if (read_op_status <= 0) {
			stream->_error_flag = WITH_ERROR;
			return SO_EOF;
		}
	}
	return_char = stream->_buffer_read[stream->_current_index_read];
	stream->_current_index_read++;
	return (unsigned char)return_char;
}

int so_fputc(int c, SO_FILE *stream)
{
	BOOL write_op_return;
	int write_op_status;
	unsigned char *write_data;
	int write_bytes_counter;
	
	stream->_dirty_buffer_write = DIRTY;
	stream->_buffer_write[stream->_current_index_write] =
		(unsigned char)c;
	stream->_current_index_write++;
	if (stream->_current_index_write == BUFFSIZE) {
		write_bytes_counter = 0;
		while(write_bytes_counter <
			stream->_current_index_write) {
				write_data = stream->_buffer_write + write_bytes_counter;
				write_op_return = WriteFile(
				   stream->_file_handle,
				   write_data,
				   BUFFSIZE - write_bytes_counter,
				   &write_op_status,
				   NULL
				);
				if (write_op_status <= 0) {
					stream->_error_flag = WITH_ERROR;
					return SO_EOF;
				}
				write_bytes_counter += write_op_status;
		}
		stream->_current_index_write = 0;
		stream->_dirty_buffer_write = CLEAN;
	}
	
	return c;
}

int so_feof(SO_FILE *stream)
{
	if (stream->_is_pipe == TRUE) {
		/* The pipe can't reach to EOF*/
		/* but it can be BROKEN */
		if (GetLastError() == ERROR_BROKEN_PIPE)
			return -1;
	}
	if (stream->_current_limit_read == -1)
		return -1;
	return SUCCESS;
}

int so_ferror(SO_FILE *stream)
{
	/* The error flag is stored in the file */
	return stream->_error_flag;
}

SO_FILE *so_popen(const char *command, const char *type)
{	
	SECURITY_ATTRIBUTES sa;
	HANDLE handle_read;
	HANDLE handle_write;

	STARTUPINFO startup_info_1;
	PROCESS_INFORMATION process_info_1;
	DWORD dw_response;
	BOOL response;
	SO_FILE *pipe_file;
	char command_environment[BUFFSIZE];
	
	strcpy_s(command_environment, BUFFSIZE, "cmd /C \"");
	strcat_s(command_environment, BUFFSIZE, command);
	strcat_s(command_environment, BUFFSIZE, "\"");

	pipe_file = (SO_FILE *) malloc(sizeof(SO_FILE));

	if (!strcmp(type, "r")) {

		ZeroMemory(&sa, sizeof(sa));
		sa.bInheritHandle = TRUE;
		ZeroMemory(&process_info_1, sizeof(process_info_1));
		response = CreatePipe(
			&handle_read, 
			&handle_write, 
			&sa, 
			0
		);
		if (response == FALSE)
			return NULL;
		SetHandleInformation(handle_read, HANDLE_FLAG_INHERIT, 0);
		RedirectHandle(&startup_info_1, handle_write, STD_OUTPUT_HANDLE);
		response = CreateProcess(
			NULL,
			command_environment,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			NULL,
			&startup_info_1,
			&process_info_1
		);
		if (response == FALSE)
			return NULL;
		CloseHandle(handle_write);

		pipe_file->_current_index_read = BUFFSIZE;
		pipe_file->_read_flag = ENABLE;
		pipe_file->_write_flag = DISABLE;
		pipe_file->_reset_position_flag = DISABLE;

		pipe_file->_current_limit_read = BUFFSIZE - 1;
		pipe_file->_current_index_write = 0;
		pipe_file->_dirty_buffer_write = CLEAN;
		pipe_file->_error_flag = NON_ERROR;
		pipe_file->_process_info = process_info_1;
		pipe_file->_is_pipe = TRUE;

		pipe_file->_file_handle = handle_read;

	}
	else if (!strcmp(type, "w")) {

		ZeroMemory(&sa, sizeof(sa));
		sa.bInheritHandle = TRUE;
		ZeroMemory(&process_info_1, sizeof(process_info_1));
		response = CreatePipe(&handle_read, &handle_write, &sa, 0);
		if (response == FALSE)
			return NULL;
		SetHandleInformation(handle_write, HANDLE_FLAG_INHERIT, 0);
		RedirectHandle(&startup_info_1, handle_write, STD_INPUT_HANDLE);
		response = CreateProcess(
			NULL,
			command_environment,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			NULL,
			&startup_info_1,
			&process_info_1
		);

		if (response == FALSE)
			return NULL;
		CloseHandle(handle_read);
		
		pipe_file->_current_index_read = BUFFSIZE;
		pipe_file->_read_flag = DISABLE;
		pipe_file->_write_flag = ENABLE;
		pipe_file->_reset_position_flag = ENABLE;

		pipe_file->_current_limit_read = BUFFSIZE - 1;
		pipe_file->_current_index_write = 0;
		pipe_file->_dirty_buffer_write = 0;
		pipe_file->_error_flag = 0;
		pipe_file->_process_info = process_info_1;
		pipe_file->_is_pipe = TRUE;


		pipe_file->_file_handle = handle_write;
	}
	return pipe_file;
}

/* Get from the lab mostly */
int so_pclose(SO_FILE *stream)
{
	int fflush_return_value;
	int close_handle_return;
	DWORD wait_response;
	
	if (stream->_reset_position_flag == ENABLE) {
		fflush_return_value = so_fflush(stream);
		if (fflush_return_value != SUCCESS) {
			stream->_error_flag = WITH_ERROR;
			free(stream);
			return SO_EOF;
		}
	}
	if (stream->_write_flag == ENABLE)
		Sleep(10000);
		
	wait_response = WaitForSingleObject(&stream->_process_info, INFINITE);
	CloseProcess(&stream->_process_info);
	
	close_handle_return = CloseHandle(stream->_file_handle);
	
	free(stream);
	if (close_handle_return)
		return SUCCESS;
	return FAIL;
}