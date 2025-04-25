package org.jna;

import java.io.InputStream;
import java.util.Properties;

public class Main {
    public static void main(String[] args) {
        Config config = loadConfig();
        LibraryRunner.runNativeLibrary(config);
    }

    private static Config loadConfig() {
        Properties properties = new Properties();
        try (InputStream input = Main.class.getClassLoader().getResourceAsStream("application.properties")) {
            if (input == null) {
                throw new RuntimeException("Unable to find application.properties");
            }
            properties.load(input);
        } catch (Exception e) {
            throw new RuntimeException("Failed to load configuration", e);
        }
        return Config.fromProperties(properties);
    }
}
