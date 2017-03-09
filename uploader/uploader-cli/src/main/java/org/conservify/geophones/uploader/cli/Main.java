package org.conservify.geophones.uploader.cli;

import org.apache.commons.cli.*;
import org.conservify.geophones.uploader.GeophoneUploaderConfiguration;
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
        options.addOption(null, "samples-per-file", false, "samples per file");
        options.addOption(null, "data", false, "data directory");
        options.addOption(null, "url", false, "upload url");
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
        properties.setProperty("samplesPerFile", cmd.getOptionValue("samples-per-file", "30720"));
        properties.setProperty("dataDirectory", cmd.getOptionValue("data", "."));
        properties.setProperty("uploadUrl", cmd.getOptionValue("url", "https://conservify.page5of4.com/geophones"));

        AnnotationConfigApplicationContext applicationContext = new AnnotationConfigApplicationContext();
        applicationContext.getEnvironment().getPropertySources().addFirst(new PropertiesPropertySource("commandLine", properties));
        applicationContext.register(DefaultConfig.class);
        applicationContext.refresh();
        applicationContext.start();

        GeophoneUploaderConfiguration configuration = applicationContext.getBean(GeophoneUploaderConfiguration.class);

        logger.info("Configuration:");
        logger.info("URL: {}", configuration.getUploadUrl());
        logger.info("Data: {}", configuration.getDataDirectory());
        logger.info("SamplesPerFile: {}", configuration.getSamplesPerFile());

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
