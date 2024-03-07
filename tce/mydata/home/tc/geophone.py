#!/usr/bin/python3

import time
import datetime
import subprocess
import pathlib
import argparse

def find_device_number():
    completed = subprocess.run(["arecord", "-l"], capture_output=True)
    lines = [l.decode('utf-8').split(":") for l in completed.stdout.splitlines() if "UMC404HD" in str(l)]
    assert len(lines) > 0, "UMC404HD not found."
    device = lines[0][0].replace("card", "").strip()
    return int(device)

def block_for_new_minute():
    started = datetime.datetime.now()
    while datetime.datetime.now().minute == started.minute:
        time.sleep(0.01)
    return datetime.datetime.now()


def consume_child_procs(children):
    for proc in children:
        code = proc.poll()
        if code is not None:
            children.remove(proc)

def main():
    parser = argparse.ArgumentParser(description="delete old data directories")
    parser.add_argument("--path", type=str)
    parser.add_argument("--done", type=str)
    args = parser.parse_args()

    device_number = find_device_number()

    device = "plughw:%d,0" % device_number

    print(f"found device {device}...")
    print(f"waiting for a new minute to start...")

    block_for_new_minute()

    running_procs = []

    while True:
        now = datetime.datetime.now()
        relative = now.strftime('%Y%m/%d/%H')
        fn = now.strftime('%Y%m%d_%H%M00.wav')
        path = args.path + "/" + relative
        print(now, path)
        pathlib.Path(path).mkdir(parents=True, exist_ok=True)
        full_path = path + "/" + fn
        completed = subprocess.run(["arecord", "-D", device, "-d", "60", "-f", "S32_LE", "-c", "3", full_path])
        consume_child_procs(running_procs)
        if completed.returncode == 0:
            running_procs.append(subprocess.Popen([args.done, full_path]))
        else:
            time.sleep(1)


if __name__ == '__main__':
    main()
