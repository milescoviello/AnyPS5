# libSceNKWebKit

## add Functional

- [WTF::sixCharacterHashStringToInteger](../../../core/libs/prx/libSceNKWebKit/src/WtfPrimitives.cpp) (libSceNKWebKit) reads the hash's characters and its terminator one at a time with `GuestCopyIn_nid_no_patch`, stopping at the first invalid one as before, so a string that can't be read throws `std::invalid_argument`, as a null or malformed one does. This is a host-safety fallback; the console's behaviour for an inaccessible string is not measured.
- [WTF::Internal::parseDoubleFromLongString](../../../core/libs/prx/libSceNKWebKit/src/WtfString.cpp) (libSceNKWebKit) copies the caller's UTF-16 units in with `GuestCopyIn_nid_no_patch`, 2048 at a time, and writes the parsed length with `GuestCopyOut_nid_no_patch`. A string that can't be read or a parsed-length output that can't be written throws `std::invalid_argument`. This is a host-safety fallback; the console's behaviour for inaccessible pointers is not measured.
