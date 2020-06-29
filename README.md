# Filesystem dump tool

This project is a simple tool to copy drives into image files. It currently
only supports copying APFS partitions into Apple sparseimage files.

The tool will only copy blocks marked as occupied in the filesystem, in order to
save space and time. The resulting sparseimage file can be mounted in macOS or
using apfs-fuse, for example.

