package org.conservify.geophones.uploader.util;

@FunctionalInterface
public interface ExponentialBackOffFunction<T> {
    T execute();
}
