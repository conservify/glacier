package org.conservify.geophones.streamer;

import java.io.File;

public class PendingFile implements Comparable<PendingFile> {
    private final File file;
    private final long date;

    public File getFile() {
        return file;
    }

    public PendingFile(File file) {
        this.file = file;
        this.date = file.lastModified();
    }

    @Override
    public int compareTo(PendingFile o) {
        return Long.compare(date, o.date);
    }
}
