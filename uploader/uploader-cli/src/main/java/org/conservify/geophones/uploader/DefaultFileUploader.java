package org.conservify.geophones.uploader;

import com.google.common.base.Stopwatch;
import com.google.common.collect.Lists;
import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Async;

import javax.annotation.PreDestroy;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;
import java.nio.charset.Charset;
import java.nio.file.*;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class DefaultFileUploader implements FileUploader {
    private static final Logger logger = LoggerFactory.getLogger(DefaultFileUploader.class);
    private final UploaderConfiguration configuration;
    private volatile boolean running = true;

    public DefaultFileUploader(UploaderConfiguration configuration) {
        this.configuration = configuration;
    }

    @Async
    @Override
    public void run() {
        Path path = FileSystems.getDefault().getPath(configuration.getDataDirectory()).toAbsolutePath();

        logger.info("Watching {}", path);

        while (running) {
            try (final WatchService watchService = FileSystems.getDefault().newWatchService()) {
                final WatchKey watchKey = path.register(watchService, StandardWatchEventKinds.ENTRY_MODIFY, StandardWatchEventKinds.ENTRY_CREATE);
                while (running) {
                    watchService.poll(1L, TimeUnit.SECONDS);
                    upload(path);
                }
            } catch (IOException e) {
                logger.error(e.getMessage(), e);

                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e1) {
                }
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
    }

    @PreDestroy
    public void stop() {
        running = false;
    }

    private void upload(Path path) {
        for (PendingFile file : getPendingFiles(path)) {
            logger.info("Uploading {}", file.getFile());

            try {
                Stopwatch timer = Stopwatch.createStarted();

                URL url = new URL(configuration.getUploadUrl());
                HttpURLConnection httpConnection = (HttpURLConnection)url.openConnection();
                httpConnection.setRequestProperty("Content-Type", "application/octet-stream");
                httpConnection.setRequestProperty("x-token", "zddgXMjr_YI2e87G0mch6tXHMupLGZ6PZ58mHOSdqJtQ566PJj8mzQ");
                httpConnection.setRequestProperty("x-timestamp", Long.toString(file.getTimestamp().getTime()));
                httpConnection.setRequestProperty("x-frequency", "512");
                httpConnection.setRequestProperty("x-input-id", "0");
                httpConnection.setRequestProperty("x-format", "float32,float32,float32");
                httpConnection.setRequestMethod("POST");
                httpConnection.setDoOutput(true);

                FileInputStream fileStream = new FileInputStream(file.getFile());
                try {
                    IOUtils.copy(fileStream, httpConnection.getOutputStream());
                    String response = IOUtils.toString(httpConnection.getInputStream(), Charset.defaultCharset());
                    logger.trace("Response: {}", response);
                }
                finally {
                    IOUtils.closeQuietly(fileStream);
                    logger.info("Done uploading {} {} ({})", file.getFile(), httpConnection.getResponseCode(), timer.stop());
                }

                archive(file);
            }
            catch (IOException e) {
                logger.error("Error uploading file", e);
            }
        }
    }

    private static final SimpleDateFormat directoryStampFormat = new SimpleDateFormat("yyyyMM/dd_HH");

    private void archive(PendingFile file) {
        String path = directoryStampFormat.format(file.getTimestamp());
        File directory = new File(new File(file.getFile().getParentFile(), "archive"), path);
        directory.mkdirs();
        File newFile = new File(directory, file.getFile().getName());
        file.getFile().renameTo(newFile);
    }

    private List<PendingFile> getPendingFiles(Path path) {
        List<PendingFile> files = Lists.newArrayList();
        for (File file : path.toFile().listFiles()) {
            try {
                if (configuration.getUploadPredicate().test(file)) {
                    if (file.isFile()) {
                        FileChannel fileChannel = FileChannel.open(file.toPath(), StandardOpenOption.WRITE, StandardOpenOption.READ);
                        try {
                            FileLock lock = fileChannel.lock();
                            lock.release();
                        }
                        finally {
                            fileChannel.close();
                        }

                        files.add(new PendingFile(file.getAbsoluteFile()));
                    }
                }
            } catch (OverlappingFileLockException e) {
                logger.trace("File locked {}", file);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        Collections.sort(files);
        Collections.reverse(files);
        return files;
    }
}
