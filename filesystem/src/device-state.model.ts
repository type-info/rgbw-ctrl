import { LightState } from "../../app/src/app/light.model";

export type WiFiStatusString =
    | "DISCONNECTED"
    | "CONNECTED"
    | "CONNECTED_NO_IP"
    | "WRONG_PASSWORD"
    | "NO_AP_FOUND"
    | "CONNECTION_FAILED"
    | "UNKNOWN";

export type AlexaIntegrationModeString =
    | "OFF"
    | "RGBW_DEVICE"
    | "RGB_DEVICE"
    | "MULTI_DEVICE";

export type BleStatusString =
    | "OFF"
    | "ADVERTISING"
    | "CONNECTED";

export type OtaStatusString =
    | "Idle"
    | "Update in progress"
    | "Update completed successfully"
    | "Update failed";

export type DeviceState = {
    "deviceName": string,
    "firmwareVersion": string,
    "heap": number,
    "wifi": {
        "details": {
            "ssid": string,
            "mac": string,
            "ip": string,
            "gateway": string,
            "subnet": string,
            "dns": string
        },
        "status": WiFiStatusString
    },
    "alexa": {
        "mode": AlexaIntegrationModeString,
        "names": string[]
    },
    "output": [LightState, LightState, LightState, LightState],
    "ble": {
        "status": BleStatusString
    },
    "ota": {
        "state": OtaStatusString
    }
}