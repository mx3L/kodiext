#include <stdio.h>

#ifndef _COMMON_H_
#define _COMMON_H_

#define MAXLINE 4096

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

ssize_t readn(int fd, void *vptr, size_t n);
ssize_t Readn(int fd, void *ptr, size_t nbytes);
ssize_t writen(int fd, const void *vptr, size_t n);
void Writen(int fd, void *ptr, size_t nbytes);

#endif
