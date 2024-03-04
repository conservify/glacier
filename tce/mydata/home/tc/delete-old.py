#!/usr/bin/python3

import os
import re
import dataclasses
import datetime
import argparse
import shutil

MB = 1024 * 1024

@dataclasses.dataclass
class DatedFile:
    path: str
    stamp: datetime.datetime

def get_dated_file(dir_path):
    m = re.search(r"(\d\d\d\d)(\d\d)(\d\d)_(\d\d)(\d\d)(\d\d)", dir_path)
    if m is None:
        return None
    
    stamp = datetime.datetime(int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5)), int(m.group(6)))
    return DatedPath(dir_path, stamp)

@dataclasses.dataclass
class DatedPath:
    path: str
    stamp: datetime.datetime

def get_dated_path(dir_path):
    m = re.search(r"(\d\d\d\d)(\d\d)\/(\d\d)\/(\d\d)", dir_path)
    if m is None:
        return None
    
    stamp = datetime.datetime(int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), 0, 0)
    return DatedPath(dir_path, stamp)

def main():
    parser = argparse.ArgumentParser(description="delete old data directories")
    parser.add_argument("--path", type=str)
    parser.add_argument("--maximum", type=int)
    parser.add_argument("--force", action='store_true')
    args = parser.parse_args()

    print(args)

    total_size = 0
    dated_files = []
    dated_dirs = []
    for dir_path, children, files in os.walk(args.path):
        dated = get_dated_path(dir_path)
        if dated:
            dir_size = 0
            for f in files:
                fp = os.path.join(dir_path, f)
                if not os.path.islink(fp):
                    file_size = os.path.getsize(fp)
                    dated_file = get_dated_file(fp)
                    if dated_file:
                        dated_files.append((dated_file, file_size))
                    dir_size += file_size
            dated_dirs.append((dated, dir_size))
            total_size += dir_size

    dated_dirs = sorted(dated_dirs, key=lambda x: x[0].stamp, reverse=True)
    dated_files = sorted(dated_files, key=lambda x: x[0].stamp, reverse=True)

    size_after = total_size
    maximum = args.maximum * MB
    while size_after > maximum and len(dated_files) > 0:
        df, size = dated_files.pop()
        size_after -= size
        print(f"deleting {df.path} ({size_after / MB} bytes after)")
        if args.force:
            shutil.rmtree(df.path)

if __name__ == '__main__':
    main()
