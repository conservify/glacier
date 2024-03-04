#!/usr/bin/python3

import os
import re
import dataclasses
import datetime
import argparse
import shutil

@dataclasses.dataclass
class DatedPath:
    path: str
    stamp: datetime.datetime

def get_dated_path(dir_path):
    m = re.search(r"(\d\d\d\d)(\d\d)\/(\d\d)\/(\d\d)", dir_path)
    if m is None:
        return None
    
    stamp = datetime.datetime(int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)))
    return DatedPath(dir_path, stamp)

def main():
    parser = argparse.ArgumentParser(description="delete old data directories")
    parser.add_argument("--path", type=str)
    parser.add_argument("--maximum", type=int)
    parser.add_argument("--force", action='store_true')
    args = parser.parse_args()

    print(args)

    total_size = 0
    dirs = []
    for dir_path, children, files in os.walk(args.path):
        dated = get_dated_path(dir_path)
        if dated:
            dir_size = 0
            for f in files:
                fp = os.path.join(dir_path, f)
                if not os.path.islink(fp):
                    dir_size += os.path.getsize(fp)
            dirs.append((dated, dir_size))
            total_size += dir_size

    dirs = sorted(dirs, key=lambda x: x[0].stamp, reverse=True)

    size_after = total_size
    MB = 1024 * 1024
    maximum = args.maximum * MB
    while size_after > maximum:
        dir, size = dirs.pop()
        size_after -= size
        print(f"deleting {dir.path} ({size_after / MB} bytes after)")
        if args.force:
            shutil.rmtree(dir.path)

if __name__ == '__main__':
    main()
