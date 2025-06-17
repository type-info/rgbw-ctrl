import {
  SimpleWiFiConnectionCredentials,
  EAPWiFiConnectionCredentials,
  WiFiConnectionDetails,
  WIFI_MAX_PASSWORD_LENGTH,
  WIFI_MAX_EAP_IDENTITY_LENGTH,
  WIFI_MAX_EAP_USERNAME_LENGTH,
  WIFI_MAX_EAP_PASSWORD_LENGTH,
  isEnterprise,
  WIFI_CONNECTION_DETAILS_LENGTH,
  WIFI_SSID_MAX_LENGTH
} from './wifi.model';
import {
  ALEXA_MAX_DEVICE_NAME_LENGTH,
  ALEXA_SETTINGS_TOTAL_LENGTH,
  AlexaIntegrationSettings
} from './alexa-integration-settings.model';
import {MAX_HTTP_PASSWORD_LENGTH, MAX_HTTP_USERNAME_LENGTH, HttpCredentials} from './http-credentials.model';

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
