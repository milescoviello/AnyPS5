# libSceLibcInternal

## add Functional

- [setiosflags, resetiosflags and setbase manipulator functions](../../../core/libs/prx/libSceLibcInternal/IosBase.cpp) (libSceLibcInternal) read the stream's format flags with `GuestCopyIn_nid_no_patch` and write them back with `GuestCopyOut_nid_no_patch`, so a stream that can't be read or written throws `std::invalid_argument`, as a null one does. This is a host-safety fallback; the console's behaviour for an inaccessible stream is not measured.
- [_Gettime](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) copies its time string in with `CopyGuestString_nid_no_patch` (up to 64 KiB, as `_Gentime` does) and writes the field length with one `GuestCopyOut_nid_no_patch`, so an unreadable string or an unwritable length throws `std::invalid_argument`. This is a host-safety fallback; the console's behaviour for an inaccessible string or length is not measured.
