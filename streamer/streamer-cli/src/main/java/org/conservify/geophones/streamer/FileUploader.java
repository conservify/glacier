package org.conservify.geophones.streamer;

import com.google.common.collect.Lists;
import org.apache.commons.io.IOUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;
import java.nio.file.*;
import java.util.Collections;
import java.util.List;
import java.util.function.Predicate;

public class FileUploader implements Runnable {
    private static final Logger logger = LoggerFactory.getLogger(FileUploader.class);
    private final Predicate<File> fileNameFilter;

    public FileUploader(Predicate<File> fileNameFilter) {
        this.fileNameFilter = fileNameFilter;
    }

    @Override
    public void run() {
        while (true) {
            Path path = FileSystems.getDefault().getPath(".");
            try (final WatchService watchService = FileSystems.getDefault().newWatchService()) {
                final WatchKey watchKey = path.register(watchService, StandardWatchEventKinds.ENTRY_MODIFY, StandardWatchEventKinds.ENTRY_CREATE);
                while (true) {
                    final WatchKey wk = watchService.take();

                    if (wk.pollEvents().size() > 0) {
                        upload(path);
                    }

                    boolean valid = wk.reset();
                    if (!valid) {
                        System.out.println("Key has been unregistered.");
                    }

                    Thread.sleep(5000);
                }
            } catch (IOException e) {
                logger.error(e.getMessage(), e);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private void upload(Path path) {
        for (PendingFile file : getPendingFiles(path)) {
            logger.info("Uploading {}...", file.getFile());
            if (!file.getFile().delete()) {
                logger.error("Error removing {}...", file.getFile());
            }
        }
    }

    private List<PendingFile> getPendingFiles(Path path) {
        List<PendingFile> files = Lists.newArrayList();
        for (File file : path.toFile().listFiles()) {
            try {
                if (fileNameFilter.test(file)) {
                    if (file.isFile()) {
                        FileChannel fileChannel = FileChannel.open(file.toPath(), StandardOpenOption.WRITE, StandardOpenOption.READ);
                        try {
                            FileLock lock = fileChannel.lock();
                            lock.release();
                        }
                        finally {
                            fileChannel.close();
                        }

                        files.add(new PendingFile(file));
                    }
                }
            } catch (OverlappingFileLockException e) {
                // logger.debug("File locked {}", file);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        Collections.sort(files);
        Collections.reverse(files);
        return files;
    }

}
