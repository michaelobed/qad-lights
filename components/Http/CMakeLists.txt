idf_component_register(SRCS "Http.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "Config" "esp_http_server" "Lighting" "Network"
                    EMBED_TXTFILES html/home.html html/styles.css html/wifisearch.html)
