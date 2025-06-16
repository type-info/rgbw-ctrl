#pragma once

#include "Arduino.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include "EspalexaDevice.h"


class Espalexa
{
    String body = "";
    uint8_t currentDeviceCount = 0;
    bool discoverable = true;
    bool udpConnected = false;
    EspalexaDevice* devices[4] = {};
    WiFiUDP espalexaUdp;
    IPAddress ipMulti;
    uint32_t mac24 = 0;
    String escapedMac = "";

    void respondToSearch()
    {
        IPAddress localIP = WiFi.localIP();
        char s[16];
        sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
        char buf[1024];
        sprintf_P(buf,PSTR("HTTP/1.1 200 OK\r\n"
                      "EXT:\r\n"
                      "CACHE-CONTROL: max-age=100\r\n"
                      "LOCATION: http://%s:80/description.xml\r\n"
                      "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n"
                      "hue-bridgeid: %s\r\n"
                      "ST: urn:schemas-upnp-org:device:basic:1\r\n"
                      "USN: uuid:2f402f80-da50-11e1-9b23-%s::upnp:rootdevice\r\n"
                      "\r\n"), s, escapedMac.c_str(), escapedMac.c_str());
        espalexaUdp.beginPacket(espalexaUdp.remoteIP(), espalexaUdp.remotePort());
        espalexaUdp.write(reinterpret_cast<uint8_t*>(buf), strlen(buf));
        espalexaUdp.endPacket();
    }

public:
    bool begin()
    {
        escapedMac = WiFi.macAddress();
        escapedMac.replace(":", "");
        escapedMac.toLowerCase();
        const String macSubStr = escapedMac.substring(6, 12);
        mac24 = strtol(macSubStr.c_str(), nullptr, 16);
        udpConnected = espalexaUdp.beginMulticast(IPAddress(239, 255, 255, 250), 1900);
        if (udpConnected)
        {
            return true;
        }
        return false;
    }

    void loop()
    {
        if (!udpConnected) return;
        const int packetSize = espalexaUdp.parsePacket();
        if (packetSize < 1) return;
        unsigned char packetBuffer[packetSize + 1];
        espalexaUdp.read(packetBuffer, packetSize);
        packetBuffer[packetSize] = 0;
        espalexaUdp.flush();
        if (!discoverable) return;
        const auto request = reinterpret_cast<const char*>(packetBuffer);
        if (strstr(request, "M-SEARCH") == nullptr) return;
        if (strstr(request, "ssdp:disc") != nullptr &&
            (strstr(request, "upnp:rootd") != nullptr ||
                strstr(request, "ssdp:all") != nullptr ||
                strstr(request, "asic:1") != nullptr))
        {
            respondToSearch();
        }
    }

    uint8_t addDevice(EspalexaDevice* d)
    {
        if (currentDeviceCount >= 4) return 0;
        if (d == nullptr) return 0;
        d->setId(currentDeviceCount);
        devices[currentDeviceCount] = d;
        return ++currentDeviceCount;
    }

    void setDiscoverable(bool d)
    {
        discoverable = d;
    }

    static uint8_t toPercent(const uint8_t bri)
    {
        uint16_t perc = bri * 100;
        return perc / 255;
    }

    AsyncWebHandler* createAlexaAsyncWebHandler()
    {
        return new AsyncAlexaWebHandler(*this);
    }

    class AsyncAlexaWebHandler final : public AsyncWebHandler
    {
        mutable String body;
        String escapedMac;
        uint32_t mac24;
        Espalexa& espalexa;

    public:
        explicit AsyncAlexaWebHandler(Espalexa& espalexa): espalexa(espalexa)
        {
            escapedMac = WiFi.macAddress();
            escapedMac.replace(":", "");
            escapedMac.toLowerCase();
            const String macSubStr = escapedMac.substring(6, 12);
            mac24 = strtol(macSubStr.c_str(), nullptr, 16);
        }

        bool canHandle(AsyncWebServerRequest* request) const override
        {
            if (const auto& url = request->url();
                url.startsWith("/description.xml") || url.startsWith("/api"))
            {
                body.clear();
                return true;
            }
            return false;
        }

        void handleRequest(AsyncWebServerRequest* request) override
        {
            espalexa.body = body;
            if (auto& url = request->url(); url == "/description.xml")
            {
                return serveDescription(request);
            }
            handleAlexaApiCall(request);
        }

        void handleBody(AsyncWebServerRequest* request,
                        uint8_t* data,
                        const size_t len,
                        const size_t index,
                        const size_t total) override
        {
            char b[len + 1];
            b[len] = 0;
            memcpy(b, data, len);
            body += b;
        }

        void serveDescription(AsyncWebServerRequest* request) const
        {
            IPAddress localIP = WiFi.localIP();
            char s[16];
            sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
            char buf[1024];
            sprintf_P(buf,PSTR("<?xml version=\"1.0\" ?>"
                          "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
                          "<specVersion><major>1</major><minor>0</minor></specVersion>"
                          "<URLBase>http://%s:80/</URLBase>"
                          "<device>"
                          "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
                          "<friendlyName>Espalexa (%s:80)</friendlyName>"
                          "<manufacturer>Royal Philips Electronics</manufacturer>"
                          "<manufacturerURL>http://www.philips.com</manufacturerURL>"
                          "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
                          "<modelName>Philips hue bridge 2012</modelName>"
                          "<modelNumber>929000226503</modelNumber>"
                          "<modelURL>http://www.meethue.com</modelURL>"
                          "<serialNumber>%s</serialNumber>"
                          "<UDN>uuid:2f402f80-da50-11e1-9b23-%s</UDN>"
                          "<presentationURL>index.html</presentationURL>"
                          "</device>"
                          "</root>"), s, s, escapedMac.c_str(), escapedMac.c_str());
            request->send(200, "text/xml", buf);
        }

        void handleAlexaApiCall(AsyncWebServerRequest* request) const
        {
            const String req = request->url();
            if (request->hasParam("body", true))
            {
                body = request->getParam("body", true)->value();
            }
            if (body.indexOf("devicetype") > 0)
            {
                body = "";
                request->send(200, "application/json",
                              F("[{\"success\":{\"username\":\"2WLEDHardQrI3WHYTHoMcXHgEspsM8ZZRpSKtBQr\"}}]"));
                return;
            }
            if ((req.indexOf("state") > 0) && (body.length() > 0))
            {
                request->send(200, "application/json", F("[{\"success\":{\"/lights/1/state/\": true}}]"));
                uint32_t devId = req.substring(req.indexOf("lights") + 7).toInt();
                unsigned idx = decodeLightKey(devId);
                if (idx >= espalexa.currentDeviceCount) return;
                EspalexaDevice* dev = espalexa.devices[idx];
                dev->setPropertyChanged(EspalexaDeviceProperty::none);
                if (body.indexOf("false") > 0)
                {
                    dev->setValue(0);
                    dev->setPropertyChanged(EspalexaDeviceProperty::off);
                    dev->doCallback();
                    return;
                }
                if (body.indexOf("true") > 0)
                {
                    dev->setValue(dev->getLastValue());
                    dev->setPropertyChanged(EspalexaDeviceProperty::on);
                }
                if (body.indexOf("bri") > 0)
                {
                    uint8_t briL = body.substring(body.indexOf("bri") + 5).toInt();
                    if (briL == 255)
                    {
                        dev->setValue(255);
                    }
                    else
                    {
                        dev->setValue(briL + 1);
                    }
                    dev->setPropertyChanged(EspalexaDeviceProperty::bri);
                }
                if (body.indexOf("xy") > 0)
                {
                    dev->setColorXY(body.substring(body.indexOf("[") + 1).toFloat(),
                                    body.substring(body.indexOf(",0") + 1).toFloat());
                    dev->setPropertyChanged(EspalexaDeviceProperty::xy);
                }
                if (body.indexOf("hue") > 0)
                {
                    dev->setColor(body.substring(body.indexOf("hue") + 5).toInt(),
                                  body.substring(body.indexOf("sat") + 5).toInt());
                    dev->setPropertyChanged(EspalexaDeviceProperty::hs);
                }
                if (body.indexOf("ct") > 0)
                {
                    dev->setColor(body.substring(body.indexOf("ct") + 4).toInt());
                    dev->setPropertyChanged(EspalexaDeviceProperty::ct);
                }
                dev->doCallback();
                return;
            }
            int pos = req.indexOf("lights");
            if (pos > 0)
            {
                int devId = req.substring(pos + 7).toInt();
                if (devId == 0)
                {
                    String jsonTemp = "{";
                    for (int i = 0; i < espalexa.currentDeviceCount; i++)
                    {
                        jsonTemp += '"';
                        jsonTemp += encodeLightKey(i);
                        jsonTemp += '"';
                        jsonTemp += ':';
                        char buf[512];
                        deviceJsonString(espalexa.devices[i], buf);
                        jsonTemp += buf;
                        if (i < espalexa.currentDeviceCount - 1) jsonTemp += ',';
                    }
                    jsonTemp += '}';
                    request->send(200, "application/json", jsonTemp);
                }
                else
                {
                    unsigned idx = decodeLightKey(devId);
                    if (idx < espalexa.currentDeviceCount)
                    {
                        char buf[512];
                        deviceJsonString(espalexa.devices[idx], buf);
                        request->send(200, "application/json", buf);
                    }
                    else
                    {
                        request->send(200, "application/json", "{}");
                    }
                }
                return;
            }
            request->send(200, "application/json", "{}");
        }

        int encodeLightKey(const uint8_t idx) const
        {
            static_assert(4 <= 128, "");
            return (mac24 << 7) | idx;
        }

        uint8_t decodeLightKey(int key) const
        {
            return ((static_cast<uint32_t>(key) >> 7) == mac24) ? (key & 127U) : 255U;
        }

        static void deviceJsonString(EspalexaDevice* dev, char* buf)
        {
            char buf_lightid[27];
            encodeLightId(dev->getId() + 1, buf_lightid);
            char buf_col[80] = "";
            if (static_cast<uint8_t>(dev->getType()) > 2)
                sprintf_P(buf_col,PSTR(",\"hue\":%u,\"sat\":%u,\"effect\":\"none\",\"xy\":[%f,%f]")
                          , dev->getHue(), dev->getSat(), dev->getX(), dev->getY());
            char buf_ct[16] = "";
            if (static_cast<uint8_t>(dev->getType()) > 1 && dev->getType() != EspalexaDeviceType::color)
                sprintf(buf_ct, ",\"ct\":%u", dev->getCt());
            char buf_cm[20] = "";
            if (static_cast<uint8_t>(dev->getType()) > 1)
                sprintf(buf_cm,PSTR("\",\"colormode\":\"%s"), modeString(dev->getColorMode()));
            sprintf_P(buf, PSTR(
                          "{\"state\":{\"on\":%s,\"bri\":%u%s%s,\"alert\":\"none%s\",\"mode\":\"homeautomation\",\"reachable\":true},"
                          "\"type\":\"%s\",\"name\":\"%s\",\"modelid\":\"%s\",\"manufacturername\":\"Philips\",\"productname\":\"E%u"
                          "\",\"uniqueid\":\"%s\",\"swversion\":\"espalexa-2.7.0\"}")
                      , (dev->getValue()) ? "true" : "false", dev->getLastValue() - 1, buf_col, buf_ct, buf_cm,
                      typeString(dev->getType()),
                      dev->getName().c_str(), modelidString(dev->getType()), static_cast<uint8_t>(dev->getType()),
                      buf_lightid);
        }

        static void encodeLightId(uint8_t idx, char* out)
        {
            uint8_t mac[6];
            WiFi.macAddress(mac);
            sprintf_P(out, PSTR("%02X:%02X:%02X:%02X:%02X:%02X-%02X-00:11"), mac[0], mac[1], mac[2], mac[3], mac[4],
                      mac[5],
                      idx);
        }

        static const char* modelidString(const EspalexaDeviceType t)
        {
            switch (t)
            {
            case EspalexaDeviceType::dimmable: return "LWB010";
            case EspalexaDeviceType::whitespectrum: return "LWT010";
            case EspalexaDeviceType::color: return "LST001";
            case EspalexaDeviceType::extendedcolor: return "LCT015";
            default: return "";
            }
        }

        static const char* typeString(const EspalexaDeviceType t)
        {
            switch (t)
            {
            case EspalexaDeviceType::dimmable: return PSTR("Dimmable light");
            case EspalexaDeviceType::whitespectrum: return PSTR("Color temperature light");
            case EspalexaDeviceType::color: return PSTR("Color light");
            case EspalexaDeviceType::extendedcolor: return PSTR("Extended color light");
            default: return "";
            }
        }

        static const char* modeString(const EspalexaColorMode m)
        {
            if (m == EspalexaColorMode::xy) return "xy";
            if (m == EspalexaColorMode::hs) return "hs";
            return "ct";
        }
    };
};
