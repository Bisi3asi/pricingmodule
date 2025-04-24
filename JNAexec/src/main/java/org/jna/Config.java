package org.jna;

import lombok.Getter;
import lombok.AllArgsConstructor;
import java.util.Properties;

@Getter
@AllArgsConstructor
public class Config {
    private String execFileDir;
    private String execFileName;

    public static Config fromProperties(Properties properties) {
        return new Config(
                properties.getProperty("execFileDir"),
                properties.getProperty("execFileName")
        );
    }
}
