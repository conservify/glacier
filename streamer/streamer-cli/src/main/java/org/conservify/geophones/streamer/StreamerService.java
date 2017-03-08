package org.conservify.geophones.streamer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.annotation.PostConstruct;

public class StreamerService {
    private static final Logger logger = LoggerFactory.getLogger(StreamerService.class);

    private final FileUploader fileUploader;

    public StreamerService(FileUploader fileUploader) {
        this.fileUploader = fileUploader;
    }

    @PostConstruct
    public void start() {
        fileUploader.run();
    }
}
