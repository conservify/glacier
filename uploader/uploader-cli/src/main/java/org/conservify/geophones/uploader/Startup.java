package org.conservify.geophones.uploader;

import javax.annotation.PostConstruct;

public class Startup {
    private final FileUploader fileUploader;

    public Startup(FileUploader fileUploader) {
        this.fileUploader = fileUploader;
    }

    @PostConstruct
    public void start() {
        fileUploader.run();
    }
}
