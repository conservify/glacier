package org.conservify.geophones.uploader;

import org.springframework.beans.factory.annotation.Value;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.function.Predicate;
import java.util.regex.Pattern;

public class UploaderConfiguration {
    private final SimpleDateFormat fileNameTimestampFormat = new SimpleDateFormat("yyyyMMdd_HHmmss");

    @Value("${dataDirectory}")
    private String dataDirectory;
    @Value("${uploadUrl}")
    private String uploadUrl;
    @Value("${uploadPattern}")
    private String uploadPattern;
    @Value("${disableArchive}")
    private boolean disableArchive;

    public String getDataDirectory() {
        return dataDirectory;
    }

    public void setDataDirectory(String dataDirectory) {
        this.dataDirectory = dataDirectory;
    }

    public String getUploadUrl() {
        return uploadUrl;
    }

    public void setUploadUrl(String uploadUrl) {
        this.uploadUrl = uploadUrl;
    }

    public void setUploadPattern(String uploadPattern) {
        this.uploadPattern = uploadPattern;
    }

    public String getUploadPattern() {
        return uploadPattern;
    }

    public void setDisableArchive(boolean disableArchive) {
        this.disableArchive = disableArchive;
    }

    public boolean isDisableArchive() {
        return disableArchive;
    }

    public Predicate<File> getUploadPredicate() {
        return new Predicate<File>() {
            @Override
            public boolean test(File file) {
                return Pattern.compile(uploadPattern).matcher(file.getName()).matches();
            }
        };
    }
}
