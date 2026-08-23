idf_component_register(SRCS "Http.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES "Config" "esp_http_server" "Lighting" "Network" "Sleep" "SwitchIO" "Update"
                    EMBED_TXTFILES html/fwupdate.html html/home.html html/restart.html html/styles.css html/wifisearch.html)
