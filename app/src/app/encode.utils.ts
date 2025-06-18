import {
  ALEXA_MAX_DEVICE_NAME_LENGTH,
  ALEXA_SETTINGS_TOTAL_LENGTH,
  AlexaIntegrationSettings
} from './alexa-integration-settings.model';
import {HttpCredentials, MAX_HTTP_PASSWORD_LENGTH, MAX_HTTP_USERNAME_LENGTH} from './http-credentials.model';
import {WebSocketMessageType} from './websocket.message';
import {
  EAPWiFiConnectionCredentials,
  isEnterprise,
  SimpleWiFiConnectionCredentials,
  WIFI_CONNECTION_DETAILS_LENGTH,
  WIFI_MAX_EAP_IDENTITY_LENGTH,
  WIFI_MAX_EAP_PASSWORD_LENGTH,
  WIFI_MAX_EAP_USERNAME_LENGTH,
  WIFI_MAX_PASSWORD_LENGTH,
  WIFI_SSID_MAX_LENGTH,
  WiFiConnectionDetails
} from './wifi.model';
import {BleStatus} from './ble.model';

export const textEncoder = new TextEncoder();

export class BufferWriter {
  private offset = 0;

  constructor(public readonly buffer: Uint8Array) {
  }

  writeUint8(value: number): void {
    this.buffer[this.offset++] = value;
  }

  writeCString(str: string, maxLength: number): void {
    const bytes = textEncoder.encode(str);
    const length = Math.min(bytes.length, maxLength);
    this.buffer.fill(0, this.offset, this.offset + maxLength + 1); // Zero out the full field
    this.buffer.set(bytes.slice(0, length), this.offset);
    this.offset += maxLength + 1;
  }

  getOffset(): number {
    return this.offset;
  }
}

export function encodeWiFiConnectionDetails(details: WiFiConnectionDetails): Uint8Array {
  const writer = new BufferWriter(new Uint8Array(WIFI_CONNECTION_DETAILS_LENGTH));
  writer.writeUint8(details.encryptionType);
  writer.writeCString(details.ssid, WIFI_SSID_MAX_LENGTH);

  if (isEnterprise(details.encryptionType) && 'identity' in details.credentials) {
    encodeEAPWiFiConnectionCredentials(details.credentials, writer);
  } else {
    encodeSimpleWiFiConnectionCredentials(details.credentials, writer);
  }

  return writer.buffer;
}

function encodeSimpleWiFiConnectionCredentials(
  credentials: SimpleWiFiConnectionCredentials,
  writer: BufferWriter
) {
  writer.writeCString(credentials.password, WIFI_MAX_PASSWORD_LENGTH);
}

function encodeEAPWiFiConnectionCredentials(
  credentials: EAPWiFiConnectionCredentials,
  writer: BufferWriter
) {
  writer.writeCString(credentials.identity, WIFI_MAX_EAP_IDENTITY_LENGTH);
  writer.writeCString(credentials.username, WIFI_MAX_EAP_USERNAME_LENGTH);
  writer.writeCString(credentials.password, WIFI_MAX_EAP_PASSWORD_LENGTH);
  writer.writeUint8(credentials.phase2Type);
}

export function encodeAlexaIntegrationSettings(settings: AlexaIntegrationSettings): Uint8Array {
  const buffer = new Uint8Array(ALEXA_SETTINGS_TOTAL_LENGTH);
  const writer = new BufferWriter(buffer);
  writer.writeUint8(settings.integrationMode);
  writer.writeCString(settings.rDeviceName, ALEXA_MAX_DEVICE_NAME_LENGTH - 1);
  writer.writeCString(settings.gDeviceName, ALEXA_MAX_DEVICE_NAME_LENGTH - 1);
  writer.writeCString(settings.bDeviceName, ALEXA_MAX_DEVICE_NAME_LENGTH - 1);
  writer.writeCString(settings.wDeviceName, ALEXA_MAX_DEVICE_NAME_LENGTH - 1);
  return buffer;
}

export function encodeHttpCredentials(credentials: HttpCredentials): Uint8Array {
  const buffer = new Uint8Array(MAX_HTTP_USERNAME_LENGTH + MAX_HTTP_PASSWORD_LENGTH + 2); // +2 for null terminators
  const writer = new BufferWriter(buffer);
  writer.writeCString(credentials.username, MAX_HTTP_USERNAME_LENGTH);
  writer.writeCString(credentials.password, MAX_HTTP_PASSWORD_LENGTH);
  return buffer;
}

export function encodeColorMessage(values: [number, number, number, number]): Uint8Array {
  const buffer = new Uint8Array(1 + 4);
  const writer = new BufferWriter(buffer);
  writer.writeUint8(WebSocketMessageType.ON_COLOR);
  values.forEach(v => writer.writeUint8(v));
  return buffer;
}

export function encodeHttpCredentialsMessage(credentials: HttpCredentials): Uint8Array {
  const credentialsBuffer = encodeHttpCredentials(credentials);
  const buffer = new Uint8Array(1 + credentialsBuffer.length);
  buffer[0] = WebSocketMessageType.ON_HTTP_CREDENTIALS;
  buffer.set(credentialsBuffer, 1);
  return buffer;
}

export function encodeDeviceNameMessage(deviceName: string, maxLength: number): Uint8Array {
  const buffer = new Uint8Array(1 + maxLength + 1);
  const writer = new BufferWriter(buffer);
  writer.writeUint8(WebSocketMessageType.ON_DEVICE_NAME);
  writer.writeCString(deviceName, maxLength);
  return buffer;
}

export function encodeWiFiConnectionDetailsMessage(details: WiFiConnectionDetails): Uint8Array {
  const detailsBuffer = encodeWiFiConnectionDetails(details);
  const buffer = new Uint8Array(1 + detailsBuffer.length);
  buffer[0] = WebSocketMessageType.ON_WIFI_DETAILS;
  buffer.set(detailsBuffer, 1);
  return buffer;
}

export function encodeAlexaIntegrationSettingsMessage(settings: AlexaIntegrationSettings): Uint8Array {
  const settingsBuffer = encodeAlexaIntegrationSettings(settings);
  const buffer = new Uint8Array(1 + settingsBuffer.length);
  buffer[0] = WebSocketMessageType.ON_ALEXA_INTEGRATION_SETTINGS;
  buffer.set(settingsBuffer, 1);
  return buffer;
}

export function encodeBleStatusMessage(status: BleStatus): Uint8Array {
  const buffer = new Uint8Array(2);
  buffer[0] = WebSocketMessageType.ON_BLE_STATUS;
  buffer[1] = status;
  return buffer;
}

export function encodeHeapMessage(): Uint8Array {
  return new Uint8Array([WebSocketMessageType.ON_HEAP]);
}

export function encodeWiFiScanStatusMessage(): Uint8Array {
  return new Uint8Array([WebSocketMessageType.ON_WIFI_SCAN_STATUS]);
}

export function encodeOtaProgressMessage(): Uint8Array {
  return new Uint8Array([WebSocketMessageType.ON_OTA_PROGRESS]);
}
