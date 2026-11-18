# libc

## add Functional

- [getenv, setenv, unsetenv and putenv](../../../core/libs/prx/libc/src/Environment.cpp) (libc) copy the caller's name, value or `putenv` text in with `CopyGuestString_nid_no_patch` before using it: names up to 64 KiB, values and `putenv` text up to 1 MiB. An unreadable string sets `errno` to `EFAULT` (14) and fails as an invalid name does (`getenv` returns null, the others -1). A longer string fails with `ENAMETOOLONG` (63), where FreeBSD's libc has no limit. `putenv` still stores the caller's string itself, as POSIX requires. This is a host-safety fallback: the console's result for an inaccessible string is not measured. Closes when: compared with the console.
