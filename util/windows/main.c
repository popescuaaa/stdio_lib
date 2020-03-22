#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <windows.h>

#include "so_stdio.h"

#define DEFAULT_BUF_SIZE 4096
typedef struct _so_file {

	unsigned char buffer_read[DEFAULT_BUF_SIZE];
	unsigned char buffer_write[DEFAULT_BUF_SIZE];
	int buffer_index_read;
	int buffer_limit_read;

	int buffer_index_write;

	int read_enable;
	int write_enable;
	int append_enable;

	int dirty_buffer_write;
	int ferror;
	BOOL pipe;
	HANDLE handle;

	PROCESS_INFORMATION pi;
} SO_FILE;
SO_FILE *so_fopen(const char *pathname, const char *mode)
{

	SO_FILE *rt_so_file = (SO_FILE *) malloc(sizeof(SO_FILE));



	rt_so_file->buffer_index_read = DEFAULT_BUF_SIZE;
	rt_so_file->buffer_limit_read = DEFAULT_BUF_SIZE - 1;
	rt_so_file->buffer_index_write = 0;
	rt_so_file->dirty_buffer_write = 0;
	rt_so_file->ferror = 0;

	if (!strcmp(mode, "r")) {
		rt_so_file->handle = CreateFile(
		pathname,
		GENERIC_READ|GENERIC_WRITE,	   /* access mode */
		FILE_SHARE_READ | FILE_SHARE_WRITE, /* sharing option */
		NULL,		   /* security attributes */
		OPEN_EXISTING,	   /* open only if it exists */
		FILE_ATTRIBUTE_NORMAL,/* file attributes */
		NULL
		);
		rt_so_file->read_enable = 1;
		rt_so_file->write_enable = 0;
		rt_so_file->append_enable = 0;
	} else if (!strcmp(mode, "r+")) {
		rt_so_file->handle = CreateFile(
		pathname,
		GENERIC_READ | GENERIC_WRITE,	   /* access mode */
		FILE_SHARE_READ | FILE_SHARE_WRITE, /* sharing option */
		NULL,		   /* security attributes */
		OPEN_EXISTING,	   /* open only if it exists */
		FILE_ATTRIBUTE_NORMAL,/* file attributes */
		NULL
		);
		rt_so_file->read_enable = 1;
		rt_so_file->write_enable = 1;
		rt_so_file->append_enable = 0;

	} else if (!strcmp(mode, "w")) {
		rt_so_file->handle = CreateFile(
		pathname,
		GENERIC_READ|GENERIC_WRITE,	   /* access mode */
		FILE_SHARE_READ | FILE_SHARE_WRITE, /* sharing option */
		NULL,		   /* security attributes */
		CREATE_ALWAYS,	   /* open new new */
		FILE_ATTRIBUTE_NORMAL,/* file attributes */
		NULL
		);
		rt_so_file->read_enable = 0;
		rt_so_file->write_enable = 1;
		rt_so_file->append_enable = 0;

	} else if (!strcmp(mode, "w+")) {
		rt_so_file->handle = CreateFile(
		pathname,
		GENERIC_READ | GENERIC_WRITE,	   /* access mode */
		FILE_SHARE_READ | FILE_SHARE_WRITE, /* sharing option */
		NULL,		   /* security attributes */
		CREATE_ALWAYS,	   /* open new new */
		FILE_ATTRIBUTE_NORMAL,/* file attributes */
		NULL
		);
		rt_so_file->read_enable = 1;
		rt_so_file->write_enable = 1;
		rt_so_file->append_enable = 0;

	} else if (!strcmp(mode, "a")) {
		rt_so_file->handle = CreateFile(
		pathname,
		GENERIC_READ|GENERIC_WRITE,	   /* access mode */
		FILE_SHARE_READ | FILE_SHARE_WRITE, /* sharing option */
		NULL,		   /* security attributes */
		OPEN_ALWAYS,	   /* open always */
		FILE_ATTRIBUTE_NORMAL,/* file attributes */
		NULL
		);
		SetFilePointer(rt_so_file->handle, 0, NULL, FILE_END);
		rt_so_file->read_enable = 0;
		rt_so_file->write_enable = 1;
		rt_so_file->append_enable = 1;

	} else if (!strcmp(mode, "a+")) {
		rt_so_file->handle = CreateFile(
		pathname,
		GENERIC_READ | GENERIC_WRITE,	   /* access mode */
		FILE_SHARE_READ | FILE_SHARE_WRITE, /* sharing option */
		NULL,		   /* security attributes */
		OPEN_ALWAYS,	   /* open always */
		FILE_ATTRIBUTE_NORMAL,/* file attributes */
		NULL
		);
		SetFilePointer(rt_so_file->handle, 0, NULL, FILE_END);
		rt_so_file->read_enable = 1;
		rt_so_file->write_enable = 1;
		rt_so_file->append_enable = 1;

	} else {
		free(rt_so_file);
		return NULL;
	}

	if (rt_so_file->handle == INVALID_HANDLE_VALUE) {
		free(rt_so_file);
		return NULL;
	}

	rt_so_file->pipe = FALSE;
	return rt_so_file;
}

int so_fflush(SO_FILE *stream)
{
	int rc;
	unsigned char *offset = stream->buffer_write;
	BOOL bRet;

	bRet = WriteFile(
	 stream->handle,          /* open file handle */
	 offset,       /* start of data to write */
	 stream->buffer_index_write, /* number of bytes to write */
	 &rc,	/* number of bytes that were written */
	 NULL            /* no overlapped structure */
	);
	if (rc == 0) {
		stream->ferror = -1;
		return SO_EOF;
	}
	if (rc == -1) {
		stream->ferror = -1;
		return SO_EOF;
	}

	stream->buffer_index_write = 0;
	stream->dirty_buffer_write = 0;

	return 0;
}

int so_fclose(SO_FILE *stream)
{
	int rc;

	if (stream->dirty_buffer_write == 1) {
		if (stream->append_enable == 1)
			SetFilePointer(stream->handle, 0, NULL, FILE_END);

		rc = so_fflush(stream);

		if (rc != 0) {
			stream->ferror = -1;
			free(stream);
			return SO_EOF;
		}
	}

	rc = CloseHandle(stream->handle);

	free(stream);
	if (rc)
		return 0;
	return -1;

}


int so_fgetc(SO_FILE *stream)
{
	int rd, rc;
	BOOL bRet;

	if (stream->buffer_index_read > stream->buffer_limit_read) {
		bRet = ReadFile(
		   stream->handle,        /* open file handle */
		   stream->buffer_read,     /* where to put data */
		   DEFAULT_BUF_SIZE,/* number of bytes to read */
		   &rc, /* number of bytes that were read */
		   NULL          /* no overlapped structure */
		);

		stream->buffer_index_read = 0;
		stream->buffer_limit_read = rc - 1;

		if (stream->pipe == TRUE) {
			if (bRet == FALSE && 
				GetLastError() == ERROR_BROKEN_PIPE)
				return SO_EOF;
		}
		if (rc == 0) {
			stream->ferror = -1;
			return SO_EOF;
		}
		if (rc == -1) {
			stream->ferror = SO_EOF;
			return SO_EOF;
		}
	}
	rd = stream->buffer_read[stream->buffer_index_read];
	stream->buffer_index_read = stream->buffer_index_read + 1;
	return (unsigned char)rd;
}

HANDLE so_fileno(SO_FILE *stream)
{
	return stream->handle;
}


size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t count = 0;
	int buf;
	size_t i;

	for (i = 0; i < nmemb * size; i++) {
		buf = so_fgetc(stream);
		if (buf == SO_EOF) {
			return count;
		}

		else {
			*((unsigned char *)ptr + i) = (unsigned char)buf;
			count = count + 1;
		}
	}
	return count;
}

int so_fputc(int c, SO_FILE *stream)
{
	int count;
	int rc;
	unsigned char *offset;
	BOOL bRet;

	stream->dirty_buffer_write = 1;
	stream->buffer_write[stream->buffer_index_write] = (unsigned char)c;
	stream->buffer_index_write = stream->buffer_index_write + 1;

	if (stream->buffer_index_write == DEFAULT_BUF_SIZE) {
		count = 0;

		while (count < stream->buffer_index_write) {

			offset = stream->buffer_write + count;

			bRet = WriteFile(
			   stream->handle,          /* open file handle */
			   offset,       /* start of data to write */
			   DEFAULT_BUF_SIZE, /* number of bytes to write */
			   &rc,		/* number of bytes that were written */
			   NULL            /* no overlapped structure */
			);
			if (rc == 0) {
				stream->ferror = -1;
				return SO_EOF;
			}
			if (rc == -1) {
				stream->ferror = -1;
				return SO_EOF;
			}
			count = count + rc;
		}
		stream->buffer_index_write = 0;
		stream->dirty_buffer_write = 0;
	}

	return c;

}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	size_t count = 0;
	int buf;
	size_t i;

	for (i = 0; i < nmemb * size; i++) {
		buf = so_fputc((int)(*((unsigned char *)ptr + i)), stream);
		if (buf == SO_EOF)
			return count;
		count = count + 1;
	}
	return count;
}


long so_ftell(SO_FILE *stream)
{
	int rb = 0, wb = 0;
	int currentPos = SetFilePointer(
		   stream->handle,
		   0,           /* offset 0 */
		   NULL,        /* no 64bytes offset */
		   FILE_CURRENT
	);

	if (stream->read_enable)
		rb = -stream->buffer_limit_read - 1
			+ stream->buffer_index_read;
	if (stream->write_enable)
		wb = stream->buffer_index_write;


	return currentPos + rb + wb;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int rc;

	if (stream->read_enable)
		stream->buffer_index_read = DEFAULT_BUF_SIZE;

	if (stream->write_enable && stream->buffer_index_write != 0) {
		so_fflush(stream);
		stream->buffer_index_write = 0;
	}


	rc = SetFilePointer(
		   stream->handle,
		   offset,           /* offset 0 */
		   NULL,        /* no 64bytes offset */
		   whence
	);
	if (rc == -1) {
		stream->ferror = -1;
		return rc;
	}

	return 0;
}

int so_ferror(SO_FILE *stream)
{
	return stream->ferror;
}


int so_feof(SO_FILE *stream)
{
	if (stream->pipe == TRUE) {
		if (GetLastError() == ERROR_BROKEN_PIPE)
			return -1;
	}
	if (stream->buffer_limit_read == -1)
		return -1;
	return 0;

}

static VOID RedirectHandle(STARTUPINFO *psi, HANDLE hFile, INT opt)
{
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	ZeroMemory(psi, sizeof(*psi));
	psi->cb = sizeof(*psi);

	psi->hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	psi->hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	psi->hStdError = GetStdHandle(STD_ERROR_HANDLE);

	psi->dwFlags |= STARTF_USESTDHANDLES;

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

SO_FILE *so_popen(const char *command, const char *type)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	STARTUPINFO si1;
	PROCESS_INFORMATION pi1;
	DWORD dwRes;
	BOOL bRes;

	char command_sh[256];
	SO_FILE *rt_so_file;


	strcpy(command_sh, "cmd /C \"");
	strcat(command_sh, command);
	strcat(command_sh, "\"");


	rt_so_file = (SO_FILE *) malloc(sizeof(SO_FILE));

	if (!strcmp(type, "r")) {

		ZeroMemory(&sa, sizeof(sa));
		sa.bInheritHandle = TRUE;

		ZeroMemory(&pi1, sizeof(pi1));

		bRes = CreatePipe(&hRead, &hWrite, &sa, 0);
		if (bRes == FALSE)
			return NULL;

		SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

		RedirectHandle(&si1, hWrite, STD_OUTPUT_HANDLE);

		bRes = CreateProcess(
			NULL,
			command_sh,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			NULL,
			&si1,
			&pi1);

		if (bRes == FALSE)
			return NULL;

		CloseHandle(hWrite);



		rt_so_file->buffer_index_read = DEFAULT_BUF_SIZE;
		rt_so_file->read_enable = 1;
		rt_so_file->write_enable = 0;
		rt_so_file->append_enable = 0;

		rt_so_file->buffer_limit_read = DEFAULT_BUF_SIZE - 1;
		rt_so_file->buffer_index_write = 0;
		rt_so_file->dirty_buffer_write = 0;
		rt_so_file->ferror = 0;
		rt_so_file->pi = pi1;
		rt_so_file->pipe = TRUE;


		rt_so_file->handle = hRead;

	}


	else if (!strcmp(type, "w")) {

		ZeroMemory(&sa, sizeof(sa));
		sa.bInheritHandle = TRUE;

		ZeroMemory(&pi1, sizeof(pi1));

		bRes = CreatePipe(&hRead, &hWrite, &sa, 0);
		if (bRes == FALSE)
			return NULL;

		SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0);

		RedirectHandle(&si1, hRead, STD_INPUT_HANDLE);

		bRes = CreateProcess(
			NULL,
			command_sh,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			NULL,
			&si1,
			&pi1);

		if (bRes == FALSE)
			return NULL;

		CloseHandle(hRead);

		rt_so_file->buffer_index_read = DEFAULT_BUF_SIZE;
		rt_so_file->read_enable = 0;
		rt_so_file->write_enable = 1;
		rt_so_file->append_enable = 1;

		rt_so_file->buffer_limit_read = DEFAULT_BUF_SIZE - 1;
		rt_so_file->buffer_index_write = 0;
		rt_so_file->dirty_buffer_write = 0;
		rt_so_file->ferror = 0;
		rt_so_file->pi = pi1;
		rt_so_file->pipe = TRUE;


		rt_so_file->handle = hWrite;
	}
	return rt_so_file;

}
static VOID CloseProcess(PROCESS_INFORMATION *lppi)
{
	CloseHandle(lppi->hThread);
	CloseHandle(lppi->hProcess);
}

int so_pclose(SO_FILE *stream)
{
	int rc;

	DWORD dwRes;

	rc = 0;

	if (stream->append_enable) {
		rc = so_fflush(stream);

		if (rc != 0) {
			stream->ferror = -1;
			free(stream);
			return SO_EOF;
		}
	}
	if (stream->write_enable)
		Sleep(10000);

	dwRes = WaitForSingleObject(&stream->pi, INFINITE);
	CloseProcess(&stream->pi);

	rc = CloseHandle(stream->handle);

	free(stream);
	if (rc)
		return 0;
	return -1;


}

