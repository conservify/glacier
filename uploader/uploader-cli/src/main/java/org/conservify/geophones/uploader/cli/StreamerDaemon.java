package org.conservify.geophones.uploader.cli;

import org.apache.commons.daemon.Daemon;
import org.apache.commons.daemon.DaemonContext;
import org.apache.commons.daemon.DaemonInitException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class StreamerDaemon implements Daemon {
    private static final Logger logger = LoggerFactory.getLogger(StreamerDaemon.class);

    private final ExecutorService executorService = Executors.newSingleThreadExecutor();

    public void init(DaemonContext context) throws DaemonInitException {
        logger.debug("Initialized with arguments {}.", String.join(", ", context.getArguments()));
    }

    public void start() throws Exception {
        logger.info("Starting...");

        this.executorService.execute(new Runnable() {
            CountDownLatch latch = new CountDownLatch(1);

            public void run() {
                try {
                    latch.await();
                } catch (InterruptedException e) {
                    logger.debug("Thread interrupted, probably means we're shutting down now.");
                }
            }
        });
    }

    public void stop() throws Exception {
        logger.info("Stopping");

        this.executorService.shutdown();
    }

    public void destroy() {
        logger.info("Destroying");
    }
}
