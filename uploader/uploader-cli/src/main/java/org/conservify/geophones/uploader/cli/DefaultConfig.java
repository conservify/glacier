package org.conservify.geophones.uploader.cli;

import org.conservify.geophones.uploader.*;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.ScopedProxyMode;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.annotation.EnableScheduling;

@Configuration
@EnableScheduling
@EnableAsync
@ComponentScan(basePackages = "org.conservify.geophones.streamer", scopedProxy = ScopedProxyMode.INTERFACES)
public class DefaultConfig {
    @Bean
    public StreamerService streamerService() {
        return new StreamerService(fileUploader());
    }

    @Bean
    public GeophoneStreamerConfiguration configuration() {
        return  new GeophoneStreamerConfiguration();
    }

    @Bean
    public PortWatcher portWatcher() {
        return new PortWatcher(configuration());
    }

    @Bean
    public FileUploader fileUploader() {
        return new DefaultFileUploader(configuration());
    }
}
