# Compiling packages

1. Boot with default image.
2. Install gcc and tools:

```
tce-load -wi zstd
tce-load -wi gcc
tce-load -wi binutils
tce-load -wi linux-6.1.y_api_headers.tcz
```

Possibly:

```
tce-load -wi base-dev
tce-load -wi glibc_base-dev
```

3. You may need to install python tools, for example `scons`

```
tce-load -wi python3.11-pip
```
