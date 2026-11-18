# libSceIcu

## add Functional

- [uloc_getLanguage, uloc_getCountry, uloc_getScript, uloc_getBaseName, uloc_getKeywordValue, uloc_canonicalize, uloc_getISO3Language and uloc_getISO3Country](../../../core/libs/prx/libSceIcu/LocaleCore.cpp) (libSceIcu) - the locale ID and keyword are copied in with `GuestCopyIn_nid_no_patch` in page-bounded 64-byte chunks, up to 4 KiB. The `UErrorCode` is read and written with one copy each, and the result with `GuestCopyOut_nid_no_patch`. An unreadable locale ID or keyword, a locale ID longer than 4 KiB, or an unwritable result sets `U_ILLEGAL_ARGUMENT_ERROR`; the ISO3 lookups return an empty string instead. An unreadable or unwritable `UErrorCode` throws `std::invalid_argument`, as in `JavaUtf8.cpp`. `uloc_getParent` is left alone while #21964 changes it. This is a host-safety fallback: the console's results for inaccessible arguments are not measured. Closes when: compared with the console.
