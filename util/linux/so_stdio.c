#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

#include "so_stdio.h"
#include "constant_values.h"
#include "utility.h"

/** FILE STRUCTURE **/
struct _so_file {
	int _file_descriptor;
	unsigned char _buffer_read[BUFFSIZE];
	unsigned char _buffer_write[BUFFSIZE];
	int _current_index_read;
	int _current_limit_read;
	int _current_index_write;
	int _read_flag;
	int _write_flag;
	int _dirty_buffer_write;
	int _error_flag;
	int _child_process_id;
};

FUNC_DECL_PREFIX SO_FILE
*so_fopen(const char *pathname, const char *mode)
{
	int _file_descriptor = DEFAULT_FD_VALUE;
	int opening_mode = open_mode(mode);
	int _read_flag;
	int _write_flag;

	switch (opening_mode) {
	case READ_ONLY_MODE:
		_file_descriptor =
			open(
				pathname,
				O_RDONLY,
				DEFAULT_ACCESS_RIGHTS
			);

		_read_flag = ENABLE;
		_write_flag = DISABLE;
		break;
	case READ_WRITE_MODE:
		_file_descriptor =
			open(
				pathname,
				O_RDWR,
				DEFAULT_ACCESS_RIGHTS
			);

		_read_flag = ENABLE;
		_write_flag = ENABLE;
		break;
	case WRITE_CREATE_TRUNCATE_MODE:
		_file_descriptor =
			open(
				pathname,
				O_WRONLY |
				O_CREAT |
				O_TRUNC,
				DEFAULT_ACCESS_RIGHTS
			);

		_read_flag = DISABLE;
		_write_flag = ENABLE;
		break;
	case READ_WRITE_CREATE_TRUNCATE_MODE:
		_file_descriptor =
			open(
				pathname,
				O_RDWR |
				O_CREAT |
				O_TRUNC,
				DEFAULT_ACCESS_RIGHTS
			);
		_read_flag = ENABLE;
		_write_flag = ENABLE;
		break;
	case WRITE_ONLY_CREATE_APPEND_MODE:
		_file_descriptor =
			open(
				pathname,
				O_WRONLY |
				O_CREAT |
				O_APPEND,
				DEFAULT_ACCESS_RIGHTS
			);
		_read_flag = DISABLE;
		_write_flag = ENABLE;
		break;
	case READ_WRITE_CREATE_APPEND_MODE:
		_file_descriptor =
			open(
				pathname,
				O_RDWR |
				O_CREAT |
				O_APPEND,
				DEFAULT_ACCESS_RIGHTS
			);
		_read_flag = ENABLE;
		_write_flag = DISABLE;
		break;
	default:
		break;
	}

	/* As specified in the hw description this should return null */
	if (_file_descriptor == DEFAULT_FD_VALUE)
		return NULL;

	SO_FILE *new_file = (SO_FILE *) malloc(sizeof(SO_FILE));

	if (new_file == NULL)
		exit(BAD_ALLOC);

	new_file->_file_descriptor = _file_descriptor;
	new_file->_current_index_read = BUFFSIZE;
	new_file->_current_limit_read = BUFFSIZE - 1;
	new_file->_current_index_write = 0;
	new_file->_dirty_buffer_write = 0;
	new_file->_error_flag = NO_ERROR;
	new_file->_read_flag = _read_flag;
	new_file->_write_flag = _write_flag;

	return new_file;
}

FUNC_DECL_PREFIX
int so_fclose(SO_FILE *stream)
{
	int closing_result;
	int so_fflush_result;

	if (stream->_dirty_buffer_write == 1) {
		so_fflush_result = so_fflush(stream);
		if (so_fflush_result != SUCCESS) {
			stream->_error_flag = WITH_ERROR;
			free(stream);
			return SO_EOF;
		}
	}
	closing_result = close(stream->_file_descriptor);
	if (closing_result != SUCCESS) {
		stream->_error_flag = WITH_ERROR;
		free(stream);
		return SO_EOF;
	}
	free(stream);
	return SUCCESS;
}

FUNC_DECL_PREFIX
int so_fileno(SO_FILE *stream)
{
	return stream->_file_descriptor;
}

FUNC_DECL_PREFIX
int so_fflush(SO_FILE *stream)
{
	int write_op_result;
	unsigned char *current_data_to_write = stream->_buffer_write;

	write_op_result = write(stream->_file_descriptor,
				current_data_to_write,
				stream->_current_index_write);
	/* Reinitialize  dirty flag and current index */
	stream->_current_index_write = 0;
	stream->_dirty_buffer_write = 0;
	if (write_op_result < 0)
		return SO_EOF;
	return SUCCESS;
}

FUNC_DECL_PREFIX
int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int lseek_op_result;

	if (stream->_read_flag == ENABLE)
		stream->_current_index_read = BUFFSIZE;

	if (stream->_write_flag == ENABLE &&
	    stream->_current_index_write != 0) {
		so_fflush(stream);
		stream->_current_index_write = 0;
	}
	lseek_op_result = lseek(stream->_file_descriptor,
				offset,
				whence);
	if (lseek_op_result < 0) {
		stream->_error_flag = WITH_ERROR;
		return lseek_op_result;
	}
	return SUCCESS;
}

FUNC_DECL_PREFIX
long so_ftell(SO_FILE *stream)
{
	int write_amount = 0;
	int read_amount = 0;
	int restart_position;

	if (stream->_read_flag == ENABLE) {
		read_amount = stream->_current_limit_read + 1 -
			      stream->_current_index_read;
	}
	if (stream->_write_flag == ENABLE)
		write_amount = stream->_current_index_write;

	restart_position = lseek(stream->_file_descriptor, 0, SEEK_CUR) -
			   read_amount +
			   write_amount;
	return restart_position;
}

FUNC_DECL_PREFIX
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t count = 0;
	int current_char;
	int index;
	int total_amount = nmemb * size;

	for (index = 0; index < total_amount; index++) {
		current_char = so_fgetc(stream);
		if (current_char == SO_EOF)
			return count / size;
		*((unsigned char *) ptr + index) = (unsigned char) current_char;
		count++;
	}
	return count / size;
}

FUNC_DECL_PREFIX
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t count = 0;
	int current_char;
	int total_amount = nmemb * size;
	int index;

	for (index = 0; index < total_amount; index++) {
		current_char = so_fputc(*((unsigned char *)ptr + index),
		stream);
		if (current_char == SO_EOF)
			return count / size;
		count++;
	}
	return count / size;
}

FUNC_DECL_PREFIX
int so_fgetc(SO_FILE *stream)
{
	int return_char;
	int read_op_value;

	if (stream->_current_index_read > stream->_current_limit_read) {
		read_op_value = read(stream->_file_descriptor,
				     stream->_buffer_read,
				     BUFFSIZE);
		stream->_current_index_read = 0;
		stream->_current_limit_read = read_op_value - 1;
		if (read_op_value == 0) {
			stream->_error_flag = WITH_ERROR;
			return SO_EOF;
		}
		if (read_op_value == -1) {
			stream->_error_flag = WITH_ERROR;
			return SO_EOF;
		}
	}
	return_char = (int) stream->_buffer_read[stream->_current_index_read];
	stream->_current_index_read++;
	return (unsigned char) return_char;
}

FUNC_DECL_PREFIX
int so_fputc(int c, SO_FILE *stream)
{
	int write_counter;
	int write_op_value;
	unsigned char *write_data;

	/* mark the dirty flag as active */
	stream->_dirty_buffer_write = 1;
	stream->_buffer_write[stream->_current_index_write] = (unsigned char)c;
	stream->_current_index_write++;

	if (stream->_current_index_write == BUFFSIZE) {
		write_counter = 0;
		while (write_counter < stream->_current_index_write) {
			write_data = stream->_buffer_write + write_counter;
			write_op_value = write(stream->_file_descriptor,
					       write_data,
					       BUFFSIZE - write_counter);
			if (write_op_value <= 0) {
				stream->_error_flag = WITH_ERROR;
				return SO_EOF;
			}
			write_counter += write_op_value;
		}
		stream->_current_index_write = 0;
		stream->_dirty_buffer_write = 0;
	}
	return c;
}

FUNC_DECL_PREFIX
int so_feof(SO_FILE *stream)
{
	if (stream->_current_limit_read == -1)
		return -1;
	return 0;
}

FUNC_DECL_PREFIX
int so_ferror(SO_FILE *stream)
{
	return stream->_error_flag;
}

FUNC_DECL_PREFIX
SO_FILE *so_popen(const char *command, const char *type)
{
	int _file_descriptor_read;
	int _file_descriptor_write;
	int pid = DEFAULT_FD_VALUE;
	int _file_descriptors_tuple[2];

	pipe(_file_descriptors_tuple);
	_file_descriptor_read = _file_descriptors_tuple[0];
	_file_descriptor_write = _file_descriptors_tuple[1];
	pid = fork();
	switch (pid) {
	case -1: {
		return NULL;
	}
	case 0: {
		if (strcmp(type, "r") == 0) {
			if (_file_descriptor_write != STDOUT_FILENO) {
				dup2(_file_descriptor_write, STDOUT_FILENO);
				close(_file_descriptor_write);
				_file_descriptor_write = STDOUT_FILENO;
			}

			close(_file_descriptor_read);

		} else if (strcmp(type, "w") == 0) {
			if (_file_descriptor_read != STDIN_FILENO) {
				dup2(_file_descriptor_read, STDIN_FILENO);
				close(_file_descriptor_read);
			}

			close(_file_descriptor_write);
		}

		execlp("/bin/sh", "sh", "-c", command, NULL);
		return NULL;
	}
	default: {
		SO_FILE *file = (SO_FILE *) malloc(sizeof(SO_FILE));

		if (file == NULL)
			exit(BAD_ALLOC);
		if (strcmp(type, "r") == 0) {
			close(_file_descriptor_write);
			file->_file_descriptor = _file_descriptor_read;
			file->_read_flag = ENABLE;
			file->_write_flag = DISABLE;
		} else if (strcmp(type, "w") == 0) {
			close(_file_descriptor_read);
			file->_file_descriptor = _file_descriptor_write;
			file->_read_flag = DISABLE;
			file->_write_flag = ENABLE;
		}
		file->_current_index_read = BUFFSIZE;
		file->_current_limit_read = BUFFSIZE - 1;
		file->_current_index_write = 0;
		file->_dirty_buffer_write = 0;
		file->_error_flag = NO_ERROR;
		file->_child_process_id = pid;
		return file;
	}

	}

}

FUNC_DECL_PREFIX
int so_pclose(SO_FILE *stream)
{
	int pid_waiting_process_result = -1;
	int waiting_status = -1;
	int child_process_id = stream->_child_process_id;

	so_fclose(stream);
	pid_waiting_process_result =
		waitpid(child_process_id,
		&waiting_status, 0);
	if (pid_waiting_process_result == -1)
		return -1;
	return waiting_status;
}
