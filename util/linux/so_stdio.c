#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "so_stdio.h"
#include "constant_values.h"
#include "utility.h"

/**
 *  FILE STRUCTURE : mult implement all the requirement froms API 
 * 
 **/

struct _so_file
{
    int _file_descriptor;
    int _current_position;

    unsigned char *_buffer;
    int _buffer_current_size;
    int _read_bytes; /* for increasing the buffer current size with appropriate values */
    int _operation_history; /* for so  fflush */

    int _error_flag; /* return somthing different from 0 if there was an error during the file operations */

};

FUNC_DECL_PREFIX SO_FILE 
*so_fopen(const char *pathname, const char *mode)
{
    int _file_descriptor = DEFAULT_FD_VALUE;
    int opening_mode = open_mode(mode);
   

    switch (opening_mode)
    {
        case READ_ONLY_MODE:
            _file_descriptor = open(pathname, O_RDONLY, DEFAULT_ACCESS_RIGHTS);
            break;
        case READ_WRITE_MODE:
            _file_descriptor = open(pathname, O_RDWR, DEFAULT_ACCESS_RIGHTS);
            break;
        case WRITE_CREATE_TRUNCATE_MODE:
            _file_descriptor = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_ACCESS_RIGHTS);
            break;
        case READ_WRITE_CREATE_TRUNCATE_MODE:
            _file_descriptor = open(pathname, O_RDWR | O_CREAT | O_TRUNC, DEFAULT_ACCESS_RIGHTS);
            break;
        case WRITE_ONLY_CREATE_APPEND_MODE:
            _file_descriptor = open(pathname, O_WRONLY | O_CREAT | O_APPEND, DEFAULT_ACCESS_RIGHTS);
            break;
        case READ_WRITE_CREATE_APPEND_MODE:
            _file_descriptor = open(pathname, O_RDWR | O_CREAT | O_APPEND, DEFAULT_ACCESS_RIGHTS);
            break;
        default:
            break;
    }

    /* As specified in the hw description this should return null */
    if (_file_descriptor == DEFAULT_FD_VALUE)
    {
        return NULL;
    }

    SO_FILE *new_file = (SO_FILE *) malloc (sizeof(SO_FILE));
    if (new_file == NULL)
    {
        exit(BAD_ALLOC);
    }

    new_file -> _buffer = (unsigned char *) calloc (BUFFSIZE, sizeof(char));
    if (new_file -> _buffer == NULL)
    {
        exit(BAD_ALLOC);
    }

    new_file -> _current_position = 0;
    new_file -> _file_descriptor = _file_descriptor;
    new_file -> _buffer_current_size = 0;
    new_file -> _read_bytes = 0;
    new_file -> _operation_history = NULL_OP;
    new_file -> _error_flag = NO_ERROR;

    return new_file;
}

/**
 *  One important aspect is that if the file 
 *  history's last operation was write one, 
 *  it must be called a fflush function, before closing it.
 * 
 **/ 
FUNC_DECL_PREFIX 
int so_fclose(SO_FILE *stream)
{   
    int closing_result;
    int so_fflush_result;

    so_fflush_result = so_fflush(stream);
    if (so_fflush_result == SO_EOF)
    {
        return SO_EOF;
    }

    closing_result = close (stream -> _file_descriptor);
    if (closing_result < 0) 
    {
        perror(CLOSING_FILE);
        exit(BAD_SYS_CALL);
    }

    if (closing_result == SUCCESS)
    {
        free(stream -> _buffer);
        free(stream);
        return closing_result;
    }

    return SO_EOF;
}

FUNC_DECL_PREFIX 
int so_fileno(SO_FILE *stream)
{
    return stream -> _file_descriptor;
}

FUNC_DECL_PREFIX 
int so_fflush(SO_FILE *stream)
{
    int write_result;

    if (stream == NULL)
    {   
        stream -> _error_flag = WITH_ERROR;
        return SO_EOF;
    }

    /* If there were data wrote into the buffer of the file */

    if (stream -> _current_position != 0)
    {
        if (stream -> _operation_history == WRITE_OP)
        {
            write_result = write(stream -> _file_descriptor,
                                stream -> _buffer,
                                stream  -> _current_position);

            if (write_result > 0)
            {
                memset(stream -> _buffer, 0, BUFFSIZE);
                stream -> _current_position = 0;
                stream -> _buffer_current_size = BUFFSIZE;

                return SUCCESS;
            }                    
        }   

        if (stream  -> _operation_history == READ_OP)
        {
            /* should clear the whole buffer and then return EOF */
            memset(stream -> _buffer, 0, BUFFSIZE);
            stream -> _current_position = 0;
            stream ->_buffer_current_size = BUFFSIZE;

            return SUCCESS;

        }

    } 
    else 
    {
        return SUCCESS;
    }
    
    stream -> _error_flag = WITH_ERROR;
    return SO_EOF;
}

FUNC_DECL_PREFIX 
int so_fseek(SO_FILE *stream, long offset, int whence)
{
    return -1;
}

FUNC_DECL_PREFIX 
long so_ftell(SO_FILE *stream)
{
    return 0;
}

FUNC_DECL_PREFIX
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int read_amount = size * nmemb;
    int index = 0;
    int read_char;
    
    for (index = 0; index < read_amount; ++index)
    {
        read_char = so_fgetc(stream);

        if (so_feof(stream) == 0)
        {
            *((char *)ptr + index) =  read_char; 
        }
        else
        {
            /* return how many did you read */
            return index / size;
        }

    }
    
   

    return nmemb / size;
}

FUNC_DECL_PREFIX
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    char *content = (char *) ptr;
    int write_amount = nmemb * size;
    int index = 0;
    char write_status;

    while(index < write_amount)
    {
        write_status = so_fputc(*(content + index), stream);
        if (write_status == SO_EOF && *(content + index) != SO_EOF)
        {
            continue;
        }

        index++;
    }   
    

    return nmemb / size;
}

FUNC_DECL_PREFIX 
int so_fgetc(SO_FILE *stream)
{
    int read_result;
    int return_char;

    stream -> _operation_history = READ_OP;

    if ((stream -> _current_position == 0 && 
         stream -> _buffer[stream -> _current_position] == '\0') ||
         stream -> _current_position == stream -> _buffer_current_size)

    {
        read_result = read(stream -> _file_descriptor, 
                       stream -> _buffer, 
                       BUFFSIZE);

        if (read_result > 0)
        {   
            if (stream -> _current_position == stream -> _buffer_current_size)
            {
                stream -> _read_bytes += read_result;
            }

            stream -> _buffer_current_size = read_result;
            stream -> _current_position = 0;
        } 
        else
        {
            stream -> _error_flag = WITH_ERROR;
            return SO_EOF;
        }
    }

     /* 
      Extend the char from the buffer, 
      from the strem -> _current_position to an int 
     */

    return_char = (int) stream -> _buffer[stream -> _current_position];
    stream -> _current_position++;

    return return_char;
}

FUNC_DECL_PREFIX 
int so_fputc(int c, SO_FILE *stream)
{
    stream -> _operation_history = WRITE_OP;

    if (stream != NULL)
    {
        if (stream -> _current_position >= stream -> _buffer_current_size)
        {
            stream -> _read_bytes += stream -> _buffer_current_size;
            if (so_fflush(stream) == SO_EOF)
            {
                return SO_EOF;
            }
        }
        stream -> _buffer[stream -> _current_position] = c;
        stream -> _current_position++;
        return c;
    }

    stream -> _error_flag = WITH_ERROR;
    return SO_EOF;
}

FUNC_DECL_PREFIX 
int so_feof(SO_FILE *stream)
{
    return stream -> _error_flag;
}

FUNC_DECL_PREFIX 
int so_ferror(SO_FILE *stream)
{
    return stream -> _error_flag;
}

FUNC_DECL_PREFIX 
SO_FILE *so_popen(const char *command, const char *type)
{
    return NULL;
}

FUNC_DECL_PREFIX 
int so_pclose(SO_FILE *stream)
{
    return 0;
}