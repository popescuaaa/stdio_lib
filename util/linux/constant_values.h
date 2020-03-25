#ifndef CONSTANT_VALUES_H
#define CONSTANT_VALUES_H

#define BUFFSIZE 4096
/* File opening modes */
#define READ_ONLY_MODE 1
#define READ_WRITE_MODE 2
#define WRITE_CREATE_TRUNCATE_MODE 3 
#define READ_WRITE_CREATE_TRUNCATE_MODE 4
#define WRITE_ONLY_CREATE_APPEND_MODE 5
#define READ_WRITE_CREATE_APPEND_MODE 6
/* Default file values */
#define DEFAULT_FD_VALUE (-1)
#define DEFAULT_ACCESS_RIGHTS 0644
/* Errors */
#define BAD_ALLOC 12
#define SO_EOF (-1)
#define BAD_SYS_CALL (-1)
/* Return values */
#define SUCCESS 0
/* Error flags */
#define NO_ERROR 0
#define WITH_ERROR (-1)
/* Read / Write flags */
#define ENABLE 1
#define DISABLE 0
#endif /* CONSTANT_VALUES_H */
