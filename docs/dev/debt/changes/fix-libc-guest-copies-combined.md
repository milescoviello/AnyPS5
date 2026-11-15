# libc

## add Unknown function info

- [xtime_get guest output](../../../core/libs/prx/libc/src/TimeExtras.cpp) (libc) - the wall-clock time is written with `GuestCopyOut_nid_no_patch`; for a non-null unwritable output the call returns 0, its result for a null output or an unsupported base, with `errno` untouched. This is a host-safety fallback; the console module writes in user space, and whether it faults on unmapped memory is unmeasured.
- [gets_s guest buffer](../../../core/libs/prx/libc/src/BoundsChecking.cpp) (libc) - the line is read into a host string and written with its terminator in one `GuestCopyOut_nid_no_patch`; an unwritable buffer (including a one-byte buffer and the end-of-file terminator) is reported as the same "bad argument" runtime-constraint violation (EINVAL) a null buffer gets, and the rest of the line is read and discarded as on the other violations, which is a host-safety fallback; whether the console module faults on unmapped memory is unmeasured.
