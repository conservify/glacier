package org.conservify.geophones.streamer;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.function.Predicate;

public class GeophoneStreamerConfiguration {
    private int samplesPerFile = 512 * 60;

    public int getSamplesPerFile() {
        return samplesPerFile;
    }

    public void setSamplesPerFile(int samplesPerFile) {
        this.samplesPerFile = samplesPerFile;
    }

    private final SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMdd_HHmmss");

    public String generateFileName() {
        return "geophone_" + formatter.format(new Date()) + ".bin";
    }

    public Predicate<File> getUploadPredicate() {
        return new Predicate<File>() {
            @Override
            public boolean test(File file) {
                return file.getName().contains("geophone_");
            }
        };
    }
}
