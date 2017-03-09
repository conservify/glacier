package org.conservify.geophones.uploader;

import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileLock;

public class GeophoneWriter {
    private static final Logger logger = LoggerFactory.getLogger(GeophoneWriter.class);

    private final GeophoneUploaderConfiguration configuration;
    private DataOutputStream dataStream;
    private FileLock lock;
    private long samplesWritten;

    public GeophoneWriter(GeophoneUploaderConfiguration configuration) {
        this.configuration = configuration;
    }

    public void write(float[] values) {
        try {
            if (dataStream == null || samplesWritten == configuration.getSamplesPerFile()) {
                if (dataStream != null) {
                    lock.release();
                    lock.close();
                    IOUtils.closeQuietly(dataStream);
                }

                String fileName = configuration.generateFileName();
                File fileInDirectory = new File(configuration.getDataDirectory(), fileName);
                logger.info("Opening {}", fileName);
                FileOutputStream fileStream = new FileOutputStream(fileInDirectory);
                dataStream = new DataOutputStream(fileStream);
                lock = fileStream.getChannel().lock();
                samplesWritten = 0;
            }

            for (int i = 0; i < values.length; ++i) {
                dataStream.writeFloat(values[i]);
            }

            samplesWritten++;
        } catch (IOException e) {
            logger.error("Error writing data", e);
        }
    }
}
