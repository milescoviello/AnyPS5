# libSceNpCommon

## add Unknown function info

- [Heap allocator getters](../../../core/libs/prx/libSceNpCommon/Runtime.cpp) - `sceNpHeapGetAllocator`, `sceNpMemoryHeapGetAllocator`, `sceNpMemoryHeapGetAllocatorEx`, `sceNpGlobalHeapGetAllocator` and `sceNpGlobalHeapGetAllocatorEx` build the allocator on the host and write it with one checked guest write. These functions return nothing, so for an output pointing at unmapped memory (or null) nothing is written and `errno` is left alone instead of faulting the host. The console's handling of such pointers was not measured.

## add Functional

- [sceNpUtilParseJid guest buffers](../../../core/libs/prx/libSceNpCommon/Strings.cpp) (libSceNpCommon) - the text is read once through checked copies in page-bounded chunks; the pieces, the `NpJid` and the measured length are built on the host and each written with one checked copy. An unreadable text or an unwritable output returns `NP_UTIL_ERROR_INVALID_ARGUMENT`, the null-text result, with `errno` kept; a piece that doesn't fit no longer leaves the earlier pieces in the buffer. A host-safety fallback, not measured on the console.
