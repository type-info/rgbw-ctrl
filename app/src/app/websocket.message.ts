// TypeScript interfaces for WebSocket messages
// These should mirror the binary message formats used in WebSocketHandler

import { AlexaIntegrationSettings } from "./alexa-integration-settings.model";
import { BleStatus } from "./ble.mode";
import { HttpCredentials } from "./http-credentials.model";
import { WiFiConnectionDetails, WiFiScanStatus, WiFiStatus } from "./wifi.model";

export enum WebSocketMessageType {
  ON_COLOR = 0,
  ON_HTTP_CREDENTIALS = 1,
  ON_DEVICE_NAME = 2,
  ON_HEAP = 3,
  ON_BLE_STATUS = 4,
  ON_WIFI_STATUS = 5,
  ON_WIFI_SCAN_STATUS = 6,
  ON_WIFI_DETAILS = 7,
  ON_OTA_PROGRESS = 8,
  ON_ALEXA_INTEGRATION_SETTINGS = 9,
}

export interface WebSocketColorMessage {
  type: WebSocketMessageType.ON_COLOR;
  values: [number, number, number, number];
}

export interface WebSocketHttpCredentialsMessage {
  type: WebSocketMessageType.ON_HTTP_CREDENTIALS;
  credentials: HttpCredentials;
}

export interface WebSocketDeviceNameMessage {
  type: WebSocketMessageType.ON_DEVICE_NAME;
  deviceName: string;
}

export interface WebSocketWiFiConnectionDetailsMessage {
  type: WebSocketMessageType.ON_WIFI_DETAILS;
  details: WiFiConnectionDetails;
}

export interface WebSocketBleStatusMessage {
  type: WebSocketMessageType.ON_BLE_STATUS;
  status: BleStatus;
}

export interface WebSocketAlexaIntegrationSettingsMessage {
  type: WebSocketMessageType.ON_ALEXA_INTEGRATION_SETTINGS;
  settings: AlexaIntegrationSettings;
}

export interface WebSocketHeapMessage {
  type: WebSocketMessageType.ON_HEAP;
  freeHeap: number;
}

export interface WebSocketWiFiStatusMessage {
  type: WebSocketMessageType.ON_WIFI_STATUS;
  status: WiFiStatus
}

export interface WebSocketWiFiScanStatusMessage {
  type: WebSocketMessageType.ON_WIFI_SCAN_STATUS;
  status: WiFiScanStatus;
}

export interface WebSocketOtaProgressMessage {
  type: WebSocketMessageType.ON_OTA_PROGRESS;
  progress: number;
}

export type WebSocketMessage =
  | WebSocketColorMessage
  | WebSocketHttpCredentialsMessage
  | WebSocketDeviceNameMessage
  | WebSocketWiFiConnectionDetailsMessage
  | WebSocketBleStatusMessage
  | WebSocketAlexaIntegrationSettingsMessage
  | WebSocketHeapMessage
  | WebSocketWiFiStatusMessage
  | WebSocketWiFiScanStatusMessage
  | WebSocketOtaProgressMessage;
