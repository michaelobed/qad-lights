//
//  Network.cpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include <cstring>
#include "esp_log.h"
#include "esp_mac.h"
#include "mdns.h"
#include "Network.hpp"

static Config& config = Config::GetInstance();

Network::Network()
{
    netIfInstance = nullptr;
}

void Network::DeInit()
{
    mdns_free();
    esp_wifi_stop();
    ESP_LOGW(__func__, "Network de-initialised.");
}

esp_err_t Network::InitAP()
{
    char defaultSsid[16] = {};
    esp_err_t err = ESP_OK;
    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    uint8_t mac[6] = {};

    err = preInit();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "WiFi net-if initialisation failed (%d)!", err);
        return err;
    }

    /* Initialise an AP netif instance. */
    netIfInstance = esp_netif_create_default_wifi_ap();
    if(netIfInstance == nullptr) 
    {
        err = ESP_FAIL;
        ESP_LOGE(__func__, "WiFi AP creation failed!");
        return err;
    }

    /* Get the MAC address to form our SSID, then initialise WiFi and copy the settings across to its config. */
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    ESP_LOGI(__func__, "Got MAC of %02x:%02x:%02x:%02x:%02x:%02x.", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(defaultSsid, "qad-lights-%02x%02x", mac[4], mac[5]);
    strncpy((char*)wifiConfig.ap.ssid, defaultSsid, MAX_SSID_LEN);
    strncpy((char*)wifiConfig.ap.password, defaultSsid, MAX_PASSPHRASE_LEN);
    wifiConfig.ap.channel = CONFIG_NETWORK_CHANNEL;
    wifiConfig.ap.ssid_len = 0;
    wifiConfig.ap.max_connection = maxConnections;
    wifiConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    err = esp_wifi_init(&initConfig);
    if(err == ESP_OK)
        err = postInit();

    return err;
}

esp_err_t Network::InitSTA()
{
    char* configString = nullptr;
    esp_err_t err = ESP_OK;
    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();

    err = preInit();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "WiFi net-if initialisation failed (%d)!", err);
        return err;
    }

    netIfInstance = esp_netif_create_default_wifi_sta();
    if(netIfInstance == nullptr)
    {
        err = ESP_FAIL;
        ESP_LOGE(__func__, "WiFi STA creation failed!");
        return err;
    }

    config.GetConfigData("networkSsid", &configString);
    memcpy(wifiConfig.sta.ssid, configString, MAX_SSID_LEN);
    config.GetConfigData("networkPsk", &configString);
    memcpy(wifiConfig.sta.password, configString, MAX_PASSPHRASE_LEN);
    wifiConfig.sta.channel = CONFIG_NETWORK_CHANNEL;
    err = esp_wifi_init(&initConfig);
    if(err == ESP_OK)
        err = postInit();
    
    return ESP_OK;
}

esp_err_t Network::postInit()
{
    char* hostname = nullptr;
    bool isSTA = config.GetConfigData("networkIsSTA");
    esp_err_t err = ESP_OK;

    /* Set up WiFi according to the desired mode and start it.
     * We actually want AP+STA rather than just AP so we can scan for other networks at the same time. */
    err = esp_wifi_set_mode(isSTA ? WIFI_MODE_STA : WIFI_MODE_APSTA);
    if(err != ESP_OK)
        return err;

    err = esp_wifi_set_config(isSTA ? WIFI_IF_STA : WIFI_IF_AP, &wifiConfig);
    if(err != ESP_OK)
        return err;

    err = esp_wifi_start();
    if(err != ESP_OK)
        return err;

    /* Start mDNS so we're not memorising the IP address just to log into this! */
    err = mdns_init();
    if(err != ESP_OK)
        return err;
    
    config.GetConfigData("networkHostname", &hostname);
    mdns_hostname_set(hostname);
    mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);
    mdns_service_instance_name_set("_http", "_tcp", "Quick-And-Dirty Lights");
    return ESP_OK;
}

esp_err_t Network::preInit()
{
    return esp_netif_init();
}

esp_err_t Network::StartSTASearch(std::vector<WifiInfo>& list)
{
    esp_err_t err = esp_wifi_scan_start(nullptr, true);
    uint16_t numSSIDs = 0;
    wifi_ap_record_t* records = nullptr;

    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not start WiFi search (%d)!", err);
        return err;
    }
    
    esp_wifi_scan_get_ap_num(&numSSIDs);
    ESP_LOGI(__func__, "Got %u SSIDs:", numSSIDs);
    records = new wifi_ap_record_t[numSSIDs];
    esp_wifi_scan_get_ap_records(&numSSIDs, records);

    /* It turns out that wifi_ap_record_t is MASSIVE, so I'm not keeping that in RAM. :')
     * Instead, we only care about two things: what is the SSID and does this SSID require a PSK?
     * That's where WifiInfo comes in. */
    list.clear();
    for(int i = 0; i < numSSIDs; i++)
    {
        WifiInfo info;
        info.needsPsk = (records[i].authmode != WIFI_AUTH_OPEN);
        memcpy(info.ssid, records[i].ssid, MAX_SSID_LEN);
        list.push_back(info);
        ESP_LOGI(__func__, "\t%d: %s%s", i + 1, info.ssid, info.needsPsk ? " (psk)" : "");
    }

    if(records != nullptr)
        delete[] records;
    return err;
}