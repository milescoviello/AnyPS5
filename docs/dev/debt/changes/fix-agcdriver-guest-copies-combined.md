# libSceAgcDriver

## add Silent stubs

- [sceAgcDriverGetCoredumpMode guest pointer](../../../core/libs/prx/libSceAgcDriver/Unimplemented.cpp) (libSceAgcDriver) - the coredump mode (always 0) is written through the guest copy helper; returning `0x8A6D0003`, the null-output error, for an unwritable pointer is a host-safety fallback, and whether the console faults or returns an argument error on unmapped memory is unmeasured.
- [sceAgcDriverAllocateToolMemoryForGpuReset and sceAgcDriverGetAllocatedToolMemoryForGpuReset guest ranges](../../../core/libs/prx/libSceAgcDriver/Unimplemented.cpp) (libSceAgcDriver) - the 12-byte range is cleared through the guest copy helper with `errno` preserved; an unwritable range is left alone and the call still returns `0x8A6D0001`, as for a null range, which is a host-safety fallback; whether the console faults on unmapped memory is unmeasured.
