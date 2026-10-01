"""Minimal COFF object rewriting for hook builds.

patch(src, dst, changes) applies, per symbol name:
  ("unbind", new_name)  references to the symbol now go to an undefined external called
                        new_name, which the linker binds elsewhere (for the hook DLL: the
                        original exe's copy). The definition stays, renamed "<name>$dead"
                        and static: a function's COMDAT section needs its defining symbol.
                        A communal (uninitialized C global) just loses its size.
  ("export", new_name)  keep the definition but make it external under new_name (lets
                        the hook table reference a static function).
"""
import struct

SYM_SIZE = 18
RELOC_SIZE = 10
SECTION_SIZE = 40
IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_STATIC = 3


def _name_at(data, off, strtab):
    raw = data[off:off + 8]
    if raw[:4] == b"\0\0\0\0":
        (stroff,) = struct.unpack_from("<I", raw, 4)
        end = data.index(b"\0", strtab + stroff)
        return data[strtab + stroff:end].decode("latin-1")
    return raw.rstrip(b"\0").decode("latin-1")


def symbols(data):
    """Yield (offset, name, value, section, storage) for each primary symbol."""
    for _, off, name, value, section, storage in _symbols(data):
        yield off, name, value, section, storage


def _symbols(data):
    sym_ptr, nsyms = struct.unpack_from("<II", data, 8)
    strtab = sym_ptr + nsyms * SYM_SIZE
    i = 0
    while i < nsyms:
        off = sym_ptr + i * SYM_SIZE
        value, section, _type, storage, naux = struct.unpack_from("<IhHBB", data, off + 8)
        yield i, off, _name_at(data, off, strtab), value, section, storage
        i += 1 + naux


def _relocations(data):
    """Offsets of every relocation's symbol-index field."""
    nsect = struct.unpack_from("<H", data, 2)[0]
    opt = struct.unpack_from("<H", data, 16)[0]
    for s in range(nsect):
        hdr = 20 + opt + s * SECTION_SIZE
        ptr = struct.unpack_from("<I", data, hdr + 24)[0]
        count = struct.unpack_from("<H", data, hdr + 32)[0]
        for r in range(count):
            yield ptr + r * RELOC_SIZE + 4


def patch(src, dst, changes):
    """Returns the names that were found and changed."""
    data = bytearray(open(src, "rb").read())
    sym_ptr, nsyms = struct.unpack_from("<II", data, 8)
    strtab = sym_ptr + nsyms * SYM_SIZE
    (strsize,) = struct.unpack_from("<I", data, strtab)
    strings = bytearray()
    new_syms = bytearray()
    redirect = {}  # old symbol index -> new symbol index
    found = set()

    def name_field(text):
        nonlocal strings
        raw = text.encode("latin-1")
        if len(raw) <= 8:
            return raw.ljust(8, b"\0")
        field = struct.pack("<II", 0, strsize + len(strings))
        strings += raw + b"\0"
        return field

    for index, off, name, value, section, storage in list(_symbols(bytes(data))):
        if name not in changes or section < 0:
            continue
        op, new_name = changes[name]
        if op == "export":
            if section == 0:
                continue  # not defined here
            data[off:off + 8] = name_field(new_name)
            data[off + 16] = IMAGE_SYM_CLASS_EXTERNAL
        elif section == 0:  # communal or already undefined: just take the new name
            data[off:off + 8] = name_field(new_name)
            struct.pack_into("<I", data, off + 8, 0)
            data[off + 16] = IMAGE_SYM_CLASS_EXTERNAL
        else:
            data[off:off + 8] = name_field(name + "$dead")
            data[off + 16] = IMAGE_SYM_CLASS_STATIC
            _type = struct.unpack_from("<H", data, off + 14)[0]
            redirect[index] = nsyms + len(new_syms) // SYM_SIZE
            new_syms += name_field(new_name) + struct.pack("<IhHBB", 0, 0, _type, IMAGE_SYM_CLASS_EXTERNAL, 0)
        found.add(name)

    for field in _relocations(bytes(data)):
        (index,) = struct.unpack_from("<I", data, field)
        if index in redirect:
            struct.pack_into("<I", data, field, redirect[index])

    # New symbols go between the old symbol table and the string table.
    out = data[:strtab] + new_syms + struct.pack("<I", strsize + len(strings)) + data[strtab + 4:] + strings
    struct.pack_into("<I", out, 12, nsyms + len(new_syms) // SYM_SIZE)
    open(dst, "wb").write(out)
    return found
