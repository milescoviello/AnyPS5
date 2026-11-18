# libSceLibreSsl3

## add Functional

- [ECDSA_SIG_get0](../../../core/libs/prx/libSceLibreSsl3/src/EcdsaSig.cpp) (libSceLibreSsl3) copies the signature in with `GuestCopyIn_nid_no_patch`, checks that every requested output is writable and only then writes `r` and `s` with `GuestCopyOut_nid_no_patch`, so a signature that can't be read or an output that can't be written fails through `NotImplemented_nid_no_patch` with neither output changed, as `RSA_get0_key` and the other key getters do. This is a host-safety fallback; the console's behaviour for inaccessible pointers is not measured.
- [X509V3_EXT_conf_nid](../../../core/libs/prx/libSceLibreSsl3/src/X509V3ExtConfNid.cpp) (libSceLibreSsl3) copies its value string in with `CopyGuestString_nid_no_patch` (64 KiB, then up to 1 MiB) before parsing it, so an unreadable value returns null, as a null one does. This is a host-safety fallback: the console's result for an inaccessible value is not measured. Closes when: compared with the console.
