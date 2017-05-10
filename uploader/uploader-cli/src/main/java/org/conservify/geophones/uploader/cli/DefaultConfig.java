package org.conservify.geophones.uploader.cli;

import org.conservify.geophones.uploader.*;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.ScopedProxyMode;
import org.springframework.context.support.PropertySourcesPlaceholderConfigurer;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.annotation.EnableScheduling;

@Configuration
@EnableScheduling
@EnableAsync
@ComponentScan(basePackages = "org.conservify.geophones.streamer", scopedProxy = ScopedProxyMode.INTERFACES)
public class DefaultConfig {
    @Bean
    public UploaderConfiguration configuration() {
        return  new UploaderConfiguration();
    }

    @Bean
    public FileUploader fileUploader() {
        return new DefaultFileUploader(configuration());
    }

    @Bean
    public Startup startup() {
        return new Startup(fileUploader());
    }

    @Bean
    public static PropertySourcesPlaceholderConfigurer propertySourcesPlaceholderConfigurer() {
        PropertySourcesPlaceholderConfigurer propertySourcesPlaceholderConfigurer = new PropertySourcesPlaceholderConfigurer();
        return propertySourcesPlaceholderConfigurer;
    }
}
