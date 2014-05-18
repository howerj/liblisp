## log.md

Logs and reports are written to here, but they should not be versioned.

Use

```bash
    make report
```

To generate the logs.

The reports generated are:

### wc.log

Word counts.

### splint.log

The result of running splint on everything under the source directory

### elf.log

Readelf on all object files.

### .i and .s files

The are temporary files used by GCC, they are saved here, but should not
be committed.
