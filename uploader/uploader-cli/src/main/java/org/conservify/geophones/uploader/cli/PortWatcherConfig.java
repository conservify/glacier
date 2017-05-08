package org.conservify.geophones.uploader.cli;

import org.conservify.geophones.uploader.GeophoneUploaderConfiguration;
import org.conservify.geophones.uploader.PortWatcher;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class PortWatcherConfig {
    @Bean
    public PortWatcher portWatcher(GeophoneUploaderConfiguration configuration) {
        return new PortWatcher(configuration);
    }
}
