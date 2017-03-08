package org.conservify.geophones.streamer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class StreamerService implements Runnable {
    private static final Logger logger = LoggerFactory.getLogger(StreamerService.class);
    private final GeophoneStreamerConfiguration configuration = new GeophoneStreamerConfiguration();
    private final Thread thread;
    private boolean running;

    public StreamerService() {
        thread = new Thread(this);
    }

    public void run() {
        logger.info("Starting...");

        running = true;

        PortWatcher portWatcher = new PortWatcher(configuration);
        FileUploader fileUploader = new FileUploader(configuration.getUploadPredicate());

        Thread thread = new Thread(fileUploader);
        thread.start();

        while (running) {
            portWatcher.find();

            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
            }
        }
    }

    public void start() {
        thread.start();
    }

    public void stop() {
        try {
            running = false;
            thread.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}
