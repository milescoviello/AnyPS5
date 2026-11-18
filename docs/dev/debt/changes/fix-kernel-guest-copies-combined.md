# libkernel

## add Unknown function info

- [dlsym symbol name](../../../core/libs/prx/libkernel/Module/src/DynamicLoader.cpp) (libkernel) - the name is copied in with `CopyGuestString_nid_no_patch` (up to 64 KiB) before the lookup; an unreadable name returns null with a `dlerror` message like the empty name, because the console's behaviour on unmapped memory is unmeasured.

## add Functional

- [sceKernelPollEventFlag and sceKernelWaitEventFlag](../../../core/libs/prx/libkernel/EventFlag/src/EventFlag.cpp) (libkernel) - writing the result pattern and reading and writing the wait timeout through `GuestCopyIn_nid_no_patch` and `GuestCopyOut_nid_no_patch`, returning `SCE_KERNEL_ERROR_EFAULT` while preserving `errno` on an inaccessible pointer, is a host-safety fallback: the console's result for such a pointer is not measured. A satisfied poll or wait has already cleared its bits when the result pattern cannot be written, and a wait whose remaining time cannot be written reports the fault after the wait completes.
- [scePthreadMutexInit](../../../core/libs/prx/libkernel/Pthread/src/Mutex.cpp) (libkernel) - reading the attribute handle and the attribute object it points to through `GuestCopyIn_nid_no_patch` (in `pthread_mutex_init` too), and checking that the mutex slot reads and writes back through the checked copies before the new mutex is stored, returning `SCE_KERNEL_ERROR_EFAULT` on an inaccessible pointer, is a host-safety fallback: the console's result for such a pointer is not measured.
- [sceKernelDebugOutText](../../../core/libs/prx/libkernel/System/src/Notification.cpp) (libkernel) copies its text in with `CopyGuestString_nid_no_patch` before writing it, so an unreadable text returns `0x8002000e`, as a null one does. Text longer than 64 KiB is cut to its first 64 KiB. This is a host-safety fallback: the console's result for inaccessible or very long text is not measured. Closes when: compared with the console.
