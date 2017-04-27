package org.conservify.geophones.uploader;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class PendingFile implements Comparable<PendingFile> {
    private static final Logger logger = LoggerFactory.getLogger(PendingFile.class);
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

    private static final SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMdd_HHmmss");
    private static final Pattern stampRe  = Pattern.compile(".*(\\d\\d\\d\\d\\d\\d\\d\\d_\\d\\d\\d\\d\\d\\d).*");

    public Date getTimestamp() {
        Matcher matcher = stampRe.matcher(file.getName());
        if (matcher.matches()) {
            try {
                return formatter.parse(matcher.group(1));
            } catch (ParseException e) {
                logger.error(String.format("Unable to parse timestamp '%s'", matcher.group(0)), e);
            }
        }
        return new Date(this.date);
    }
}
