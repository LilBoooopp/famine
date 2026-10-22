#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

unsigned long syscall0(long n);
unsigned long syscall1(long n, long a1);
unsigned long syscall2(long n, long a1, long a2);
unsigned long syscall3(long n, long a1, long a2, long a3);
unsigned long syscall4(long n, long a1, long a2, long a3, long a4);
unsigned long syscall5(long n, long a1, long a2, long a3, long a4, long a5);
unsigned long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);

#define __SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
#define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__,7,6,5,4,3,2,1,0,)
#define __SYSCALL_CONCAT_X(a,b) a##b
#define __SYSCALL_CONCAT(a,b) __SYSCALL_CONCAT_X(a,b)
#define __SYSCALL_DISP(b,...) __SYSCALL_CONCAT(b,__SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define syscall(...) __SYSCALL_DISP(syscall,__VA_ARGS__)
