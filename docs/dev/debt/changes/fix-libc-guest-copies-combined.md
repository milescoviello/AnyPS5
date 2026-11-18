# libc

## add Unknown function info

- [xtime_get guest output](../../../core/libs/prx/libc/src/TimeExtras.cpp) (libc) - the wall-clock time is written with `GuestCopyOut_nid_no_patch`; for a non-null unwritable output the call returns 0, its result for a null output or an unsupported base, with `errno` untouched. This is a host-safety fallback; the console module writes in user space, and whether it faults on unmapped memory is unmeasured.
- [gets_s guest buffer](../../../core/libs/prx/libc/src/BoundsChecking.cpp) (libc) - the line is read into a host string and written with its terminator in one `GuestCopyOut_nid_no_patch`; an unwritable buffer (including a one-byte buffer and the end-of-file terminator) is reported as the same "bad argument" runtime-constraint violation (EINVAL) a null buffer gets, and the rest of the line is read and discarded as on the other violations, which is a host-safety fallback; whether the console module faults on unmapped memory is unmeasured.

## add Functional

- [_Thrd_join and std::__sce_v2::_Thrd_join](../../../core/libs/prx/libc/src/Threads.cpp) (libc) - a non-null exit-code output that cannot be written returns `thrd_error` (4) before joining, so the thread stays joinable. An `Xtime` argument of `_Cnd_timedwait`, `_Thrd_sleep` or the `__sce_v2` `Timespec64` helpers that cannot be read or written throws `std::invalid_argument`, as a null one does. Both are host-safety fallbacks: the console's behaviour for an inaccessible pointer is not measured.
- [mktime, timelocal](../../../core/libs/prx/libc/src/Time.cpp) (libc) copy the caller's `struct tm` in with `GuestCopyIn_nid_no_patch` and write the normalized result back with `GuestCopyOut_nid_no_patch`; `timelocal` writes its `tm_isdst = -1` the same way. A null `struct tm`, or one that can't be read or written, returns -1 with `errno` set to `EFAULT`, as `time` does for an output it can't write. This is a host-safety fallback: FreeBSD dereferences the pointer, and the console's behaviour for an inaccessible one is not measured.
