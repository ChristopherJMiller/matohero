"""Minimal COFF object rewriting for hook builds.

patch(src, dst, changes) applies, per symbol name:
  ("unbind", new_name)  make it an undefined external called new_name, so the linker
                        binds it elsewhere (for the hook DLL: the original exe's copy).
                        The defining bytes stay in the object, unreferenced.
  ("export", new_name)  keep the definition but make it external under new_name (lets
                        the hook table reference a static function).
"""
import struct

SYM_SIZE = 18
IMAGE_SYM_CLASS_EXTERNAL = 2


def symbols(data):
    """Yield (offset, name, value, section, storage) for each primary symbol."""
    sym_ptr, nsyms = struct.unpack_from("<II", data, 8)
    strtab = sym_ptr + nsyms * SYM_SIZE
    i = 0
    while i < nsyms:
        off = sym_ptr + i * SYM_SIZE
        raw = data[off:off + 8]
        if raw[:4] == b"\0\0\0\0":
            (stroff,) = struct.unpack_from("<I", raw, 4)
            end = data.index(b"\0", strtab + stroff)
            name = data[strtab + stroff:end].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        value, section, _type, storage, naux = struct.unpack_from("<IhHBB", data, off + 8)
        yield off, name, value, section, storage
        i += 1 + naux


def patch(src, dst, changes):
    """Returns the names that were found and changed."""
    data = bytearray(open(src, "rb").read())
    sym_ptr, nsyms = struct.unpack_from("<II", data, 8)
    strtab = sym_ptr + nsyms * SYM_SIZE
    (strsize,) = struct.unpack_from("<I", data, strtab)
    extra = bytearray()
    found = set()
    for off, name, value, section, storage in list(symbols(bytes(data))):
        if name not in changes or section < 0:
            continue
        op, new_name = changes[name]
        if op == "export" and section == 0:
            continue  # nothing to export: not defined here
        new = new_name.encode("latin-1")
        if len(new) <= 8:
            data[off:off + 8] = new.ljust(8, b"\0")
        else:
            data[off:off + 8] = struct.pack("<II", 0, strsize + len(extra))
            extra += new + b"\0"
        if op == "unbind":
            struct.pack_into("<Ih", data, off + 8, 0, 0)  # value 0, section UNDEFINED
        data[off + 16] = IMAGE_SYM_CLASS_EXTERNAL
        found.add(name)
    if extra:
        struct.pack_into("<I", data, strtab, strsize + len(extra))
        data += extra
    open(dst, "wb").write(data)
    return found
