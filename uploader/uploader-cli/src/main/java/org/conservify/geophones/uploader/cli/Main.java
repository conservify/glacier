package org.conservify.geophones.uploader.cli;

import org.apache.commons.cli.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.*;

import java.util.concurrent.CountDownLatch;

public class Main {
    private static final Logger logger = LoggerFactory.getLogger(Main.class);

    public static void main(String[] args) throws ParseException {
        Options options = new Options();
        options.addOption(null, "help", false, "display this message");

        org.apache.commons.cli.CommandLineParser parser = new DefaultParser();
        CommandLine cmd = parser.parse(options, args);

        if (cmd.hasOption("help")) {
            System.out.println();
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp("streamer-cli", options);
            return;
        }

        AnnotationConfigApplicationContext applicationContext = new AnnotationConfigApplicationContext(DefaultConfig.class);
        applicationContext.start();

        CountDownLatch latch = new CountDownLatch(1);

        try {
            latch.await();
        } catch (InterruptedException e) {
            logger.error(e.getMessage(), e);
        }

        applicationContext.stop();
        applicationContext.close();
    }
}
