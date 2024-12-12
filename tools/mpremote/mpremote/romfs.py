# MIT license; Copyright (c) 2022 Damien P. George

import struct, sys, os


class VfsRomWriter:
    ROMFS_HEADER = b"\xd2\xcd\x31"

    def __init__(self):
        self._dir_stack = [(None, bytearray())]
        self.opendir("")

    def _encode_uint(self, value):
        encoded = [value & 0x7F]
        value >>= 7
        while value != 0:
            encoded.insert(0, 0x80 | (value & 0x7F))
            value >>= 7
        return bytes(encoded)

    def _pack(self, kind, payload):
        return self._encode_uint(kind) + self._encode_uint(len(payload)) + payload

    def finalise(self):
        self.closedir()
        _, data = self._dir_stack.pop()
        encoded_kind = VfsRomWriter.ROMFS_HEADER
        encoded_len = self._encode_uint(len(data))
        if (len(encoded_kind) + len(encoded_len) + len(data)) % 2 == 1:
            encoded_len = b"\x80" + encoded_len
        data = encoded_kind + encoded_len + data
        return data

    def opendir(self, dirname):
        self._dir_stack.append((dirname, bytearray()))

    def closedir(self):
        dirname, dirdata = self._dir_stack.pop()
        dirdata = self._encode_uint(len(dirname)) + bytes(dirname, "ascii") + dirdata
        self._dir_stack[-1][1].extend(self._pack(1, dirdata))

    def mkfile(self, filename, filedata):
        filename = bytes(filename, "ascii")
        payload = self._encode_uint(len(filename))
        payload += filename
        payload += self._encode_uint(len(filedata))
        payload += filedata
        self._dir_stack[-1][1].extend(self._pack(2, payload))


def copy_recursively(vfs, src_dir, depth):
    assert src_dir.endswith("/")
    DIR = 1 << 14
    for name in os.listdir(src_dir):
        if name in (".", ".."):
            continue
        src_name = src_dir + name
        st = os.stat(src_name)
        if st[0] & DIR:
            print(" " * 4 * depth + "|-", name + "/")
            vfs.opendir(name)
            copy_recursively(vfs, src_name + "/", depth + 1)
            vfs.closedir()
        else:
            # todo mpy-cross .py files
            print(" " * 4 * depth + "|-", name)
            with open(src_name, "rb") as src:
                vfs.mkfile(name, src.read())


def make_romfs(src_dir, dest_dir):
    if not src_dir.endswith("/"):
        src_dir += "/"

    vfs = VfsRomWriter()

    # Build the filesystem recursively.
    print("Building romfs filesystem, source directory: {}".format(src_dir))
    try:
        copy_recursively(vfs, src_dir, 0)
    except OSError as er:
        if er.args[0] == 28:  # ENOSPC
            print("Error: not enough space on filesystem", file=sys.stderr)
            sys.exit(1)
        else:
            print("Error: OSError {}".format(er.args[0]), file=sys.stderr)
            sys.exit(1)

    return vfs.finalise()


def main():
    if len(sys.argv) != 2:
        print("usage: {} <dir>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)

    # Parse arguments.
    dir = sys.argv[1]

    romfs = make_romfs(dir, "/")

    # Save the block device data.
    output = dir.rstrip("/") + ".romfs"
    print("Writing filesystem image to {}".format(output))
    with open(output, "wb") as f:
        f.write(romfs)


if __name__ == "__main__":
    main()
