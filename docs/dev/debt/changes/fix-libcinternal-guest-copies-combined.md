# libSceLibcInternal

## add Functional

- [setiosflags, resetiosflags and setbase manipulator functions](../../../core/libs/prx/libSceLibcInternal/IosBase.cpp) (libSceLibcInternal) read the stream's format flags with `GuestCopyIn_nid_no_patch` and write them back with `GuestCopyOut_nid_no_patch`, so a stream that can't be read or written throws `std::invalid_argument`, as a null one does. This is a host-safety fallback; the console's behaviour for an inaccessible stream is not measured.
- [_Gettime](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) copies its time string in with `CopyGuestString_nid_no_patch` (up to 64 KiB, as `_Gentime` does) and writes the field length with one `GuestCopyOut_nid_no_patch`, so an unreadable string or an unwritable length throws `std::invalid_argument`. This is a host-safety fallback; the console's behaviour for an inaccessible string or length is not measured.
- [_Mtxinit](../../../core/libs/prx/libSceLibcInternal/Mutex.cpp) (libSceLibcInternal) copies at most the 32 bytes of the name it labels the mutex with, through `CopyGuestString_nid_no_patch` or one `GuestCopyIn_nid_no_patch`, so an unreadable name returns 1 before any mutex is made. This is a host-safety fallback: the console's result for an inaccessible name is not measured. Closes when: compared with the console.
