package org.conservify.geophones.uploader;

import org.springframework.beans.factory.annotation.Value;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.function.Predicate;

public class GeophoneUploaderConfiguration {
    private final SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMdd_HHmmss");
    @Value("${samplesPerFile}")
    private int samplesPerFile;
    @Value("${dataDirectory}")
    private String dataDirectory;
    @Value("${uploadUrl}")
    private String uploadUrl;

    public int getSamplesPerFile() {
        return samplesPerFile;
    }

    public void setSamplesPerFile(int samplesPerFile) {
        this.samplesPerFile = samplesPerFile;
    }

    public String getDataDirectory() {
        return dataDirectory;
    }

    public void setDataDirectory(String dataDirectory) {
        this.dataDirectory = dataDirectory;
    }

    public void setUploadUrl(String uploadUrl) {
        this.uploadUrl = uploadUrl;
    }

    public String getUploadUrl() {
        return uploadUrl;
    }

    public int getBaudRate() {
        return 115200;
    }

    public long getActivityTimeout() {
        return 5 * 1000;
    }

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
