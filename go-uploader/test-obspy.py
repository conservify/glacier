from obspy import read

st = read("test.evt")

with open("python-data.csv", "w") as file:
    time = 0.0
    for sample in range(0, st[0].stats.npts):
        for stream in [ st[0], st[1], st[2] ]:
            file.write(",".join([
                str(time),
                str(st[0].data[sample]),
                str(st[1].data[sample]),
                str(st[2].data[sample])
            ]) + "\n")

        time += 1/200.0
