import {
  EAPWiFiConnectionCredentials,
  isEnterprise,
  SimpleWiFiConnectionCredentials,
  WIFI_MAX_EAP_IDENTITY_LENGTH,
  WIFI_MAX_EAP_PASSWORD_LENGTH,
  WIFI_MAX_EAP_USERNAME_LENGTH,
  WIFI_MAX_PASSWORD_LENGTH,
  WIFI_SSID_MAX_LENGTH,
  WiFiConnectionDetails,
  WiFiDetails,
  WiFiEncryptionType,
  WiFiNetwork,
  WiFiPhaseTwoType,
  WiFiScanStatus,
  WiFiStatus
} from './wifi.model';
import {ALEXA_MAX_DEVICE_NAME_LENGTH, AlexaIntegrationSettings} from './alexa-integration-settings.model';
import {HttpCredentials} from './http-credentials.model';
import {WebSocketBleStatusMessage, WebSocketColorMessage, WebSocketMessageType} from './websocket.message';
import {LightState} from '../app/light.model';

export const textDecoder = new TextDecoder('utf-8');

export function decodeCString(bytes: Uint8Array): string {
  const nullIdx = bytes.indexOf(0);
  return nullIdx === 0 ? '' : textDecoder.decode(bytes.subarray(0, nullIdx === -1 ? bytes.length : nullIdx));
}

function macBytesToString(mac: Uint8Array): string {
  return Array.from(mac)
    .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
    .join(':');
}

class BufferReader {
  private offset = 0;

  constructor(private buffer: Uint8Array) {
  }

  readByte(): number {
    return this.buffer[this.offset++];
  }

  readUint32(): number {
    const value =
      this.buffer[this.offset] |
      (this.buffer[this.offset + 1] << 8) |
      (this.buffer[this.offset + 2] << 16) |
      (this.buffer[this.offset + 3] << 24);
    this.offset += 4;
    return value >>> 0;
  }

  readBytes(length: number): Uint8Array {
    const bytes = this.buffer.subarray(this.offset, this.offset + length);
    this.offset += length;
    return bytes;
  }

  readCString(length: number): string {
    return decodeCString(this.readBytes(length));
  }
}

export function decodeWiFiStatus(buffer: Uint8Array) {
  return buffer[0] as WiFiStatus;
}

export function decodeWiFiScanStatus(buffer: Uint8Array) {
  return buffer[0] as WiFiScanStatus;
}

export function decodeWiFiDetails(buffer: Uint8Array): WiFiDetails {
  const reader = new BufferReader(buffer);

  const ssid = reader.readCString(WIFI_SSID_MAX_LENGTH + 1);
  const mac = macBytesToString(reader.readBytes(6));
  const ip = reader.readUint32();
  const gateway = reader.readUint32();
  const subnet = reader.readUint32();
  const dns = reader.readUint32();

  return {ssid, mac, ip, gateway, subnet, dns};
}

export function decodeWiFiScanResult(buffer: Uint8Array): WiFiNetwork[] {
  const reader = new BufferReader(buffer);
  const resultCount = reader.readByte();
  const networks: WiFiNetwork[] = [];

  for (let i = 0; i < resultCount; i++) {
    const encryptionType = reader.readByte() as WiFiEncryptionType;
    const ssid = reader.readCString(WIFI_SSID_MAX_LENGTH + 1);
    networks.push({ssid, encryptionType});
  }
  return networks;
}

export function decodeWiFiConnectionDetails(buffer: Uint8Array): WiFiConnectionDetails {
  const reader = new BufferReader(buffer);
  const encryptionType = reader.readByte() as WiFiEncryptionType;
  const ssid = reader.readCString(WIFI_SSID_MAX_LENGTH + 1);
  let credentials: SimpleWiFiConnectionCredentials | EAPWiFiConnectionCredentials;
  if (isEnterprise(encryptionType)) {
    credentials = decodeEAPWiFiConnectionDetails(reader);
  } else {
    credentials = decodeSimpleWiFiConnectionDetails(reader);
  }
  return {encryptionType, ssid, credentials};
}

function decodeSimpleWiFiConnectionDetails(reader: BufferReader): SimpleWiFiConnectionCredentials {
  const password = reader.readCString(WIFI_MAX_PASSWORD_LENGTH + 1);
  return {password};
}

export function decodeEAPWiFiConnectionDetails(reader: BufferReader): EAPWiFiConnectionCredentials {
  const password = reader.readCString(WIFI_MAX_EAP_PASSWORD_LENGTH + 1);
  const identity = reader.readCString(WIFI_MAX_EAP_IDENTITY_LENGTH + 1);
  const username = reader.readCString(WIFI_MAX_EAP_USERNAME_LENGTH + 1);
  const phase2Type = reader.readByte() as WiFiPhaseTwoType;

  return {phase2Type, identity, username, password};
}

export function decodeAlexaIntegrationSettings(buffer: Uint8Array): AlexaIntegrationSettings {
  const reader = new BufferReader(buffer);
  const integrationMode = reader.readByte();

  const rDeviceName = reader.readCString(ALEXA_MAX_DEVICE_NAME_LENGTH);
  const gDeviceName = reader.readCString(ALEXA_MAX_DEVICE_NAME_LENGTH);
  const bDeviceName = reader.readCString(ALEXA_MAX_DEVICE_NAME_LENGTH);
  const wDeviceName = reader.readCString(ALEXA_MAX_DEVICE_NAME_LENGTH);

  return {integrationMode, rDeviceName, gDeviceName, bDeviceName, wDeviceName};
}

export function decodeHttpCredentials(buffer: Uint8Array): HttpCredentials {
  const reader = new BufferReader(buffer);
  const username = reader.readCString(WIFI_SSID_MAX_LENGTH + 1);
  const password = reader.readCString(WIFI_MAX_PASSWORD_LENGTH + 1);
  return {username, password};
}

export function decodeLightState(buffer: Uint8Array): LightState {
  if (buffer.length !== 2) {
    throw new Error(`Invalid light state length: ${buffer.length}`);
  }
  return {on: buffer[0] !== 0, value: buffer[1]};
}

export function decodeWebSocketOnBleStatusMessage(buffer: ArrayBuffer): WebSocketBleStatusMessage {
  const data = new Uint8Array(buffer);
  if (data.length !== 2) {
    throw new Error(`Invalid BLE status message length: ${data.length}`);
  }
  return {type: data[0], status: data[1]};
}

export function decodeWebSocketOnColorMessage(buffer: ArrayBuffer): WebSocketColorMessage {
  const data = new Uint8Array(buffer);
  if (data.length !== 4 * 2 + 1) {
    throw new Error(`Invalid color message length: ${data.length}`);
  }

  const values: [LightState, LightState, LightState, LightState] = [
    decodeLightState(data.subarray(1, 3)),
    decodeLightState(data.subarray(3, 5)),
    decodeLightState(data.subarray(5, 7)),
    decodeLightState(data.subarray(7, 9))
  ];

  return {type: WebSocketMessageType.ON_COLOR, values};
}
