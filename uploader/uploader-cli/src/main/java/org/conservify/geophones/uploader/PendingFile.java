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

    TimeStampFormat timeStampParser = new TimeStampFormat(new SimpleDateFormat("yyyyMMddHHmmss"), Pattern.compile(".*(\\d\\d\\d\\d\\d\\d\\d\\d\\d\\d\\d\\d\\d\\d).*"));

    public Date getTimestamp() {
        // Remove _'s here because some files passing through have them between the date and time.
        Date parsed = timeStampParser.parse(file.getName().replace("_", ""));
        if (parsed != null) {
            return parsed;
        }
        return new Date(this.date);
    }

    public static class TimeStampFormat {
        private SimpleDateFormat formatter;
        private Pattern pattern;

        public TimeStampFormat(SimpleDateFormat formatter, Pattern pattern) {
            this.formatter = formatter;
            this.pattern = pattern;
        }

        public Date parse(String name) {
            Matcher matcher = pattern.matcher(name);
            if (matcher.matches()) {
                try {
                    return formatter.parse(matcher.group(1));
                } catch (ParseException e) {
                    logger.error(String.format("Unable to parse timestamp '%s'", matcher.group(0)), e);
                }
            }
            return null;
        }
    }
}
