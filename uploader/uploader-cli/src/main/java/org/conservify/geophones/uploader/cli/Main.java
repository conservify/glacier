package org.conservify.geophones.uploader.cli;

import org.apache.commons.cli.*;
import org.conservify.geophones.uploader.UploaderConfiguration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.*;
import org.springframework.core.env.PropertiesPropertySource;

import java.util.Properties;
import java.util.concurrent.CountDownLatch;

public class Main {
    private static final Logger logger = LoggerFactory.getLogger(Main.class);

    public static void main(String[] args) throws ParseException {
        Options options = new Options();
        options.addOption(null, "data", true, "data directory");
        options.addOption(null, "url", true, "upload url");
        options.addOption(null, "pattern", true, "upload pattern");
        options.addOption(null, "help", false, "display this message");

        org.apache.commons.cli.CommandLineParser parser = new DefaultParser();
        CommandLine cmd = parser.parse(options, args);

        if (cmd.hasOption("help")) {
            System.out.println();
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp("streamer-cli", options);
            return;
        }

        Properties properties = new Properties();
        properties.setProperty("dataDirectory", cmd.getOptionValue("data", "."));
        properties.setProperty("uploadUrl", cmd.getOptionValue("url", "https://code.conservify.org/geophones"));
        properties.setProperty("uploadPattern", cmd.getOptionValue("pattern","(.+)_(\\d{8})_(\\d{6}).bin"));

        AnnotationConfigApplicationContext applicationContext = new AnnotationConfigApplicationContext();
        applicationContext.getEnvironment().getPropertySources().addFirst(new PropertiesPropertySource("commandLine", properties));
        applicationContext.register(DefaultConfig.class);
        applicationContext.refresh();
        applicationContext.start();

        UploaderConfiguration configuration = applicationContext.getBean(UploaderConfiguration.class);

        logger.info("Configuration:");
        logger.info("Data: {}", configuration.getDataDirectory());
        logger.info("URL: {}", configuration.getUploadUrl());
        logger.info("Pattern: {}", configuration.getUploadPattern());

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
