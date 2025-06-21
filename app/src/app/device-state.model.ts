import {OtaStatusString} from './ota.model';
import {LightState} from './light.model';

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



export type OtaState = {
    status: OtaStatusString,
    totalBytesExpected: number,
    totalBytesReceived: number,
}

export type WiFiDetailsState = {
    ssid: string,
    mac: string,
    ip: string,
    gateway: string,
    subnet: string,
    dns: string
};


export type DeviceState = {
    "deviceName": string,
    "firmwareVersion": string,
    "heap": number,
    "wifi": {
        "details": WiFiDetailsState,
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
    "ota": OtaState
}
