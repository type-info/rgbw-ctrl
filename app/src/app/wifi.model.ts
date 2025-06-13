export const WIFI_SSID_MAX_LENGTH = 32;
export const WIFI_MAX_SSID_LENGTH = 32;
export const WIFI_MAX_PASSWORD_LENGTH = 64;
export const WIFI_MAX_EAP_IDENTITY_LENGTH = 128;
export const WIFI_MAX_EAP_USERNAME_LENGTH = 128;
export const WIFI_MAX_EAP_PASSWORD_LENGTH = 128;
export const WIFI_PHASE_TWO_TYPE_LENGTH = 1;
export const EAP_WIFI_CONNECTION_CREDENTIALS_LENGTH = WIFI_MAX_EAP_IDENTITY_LENGTH + 1 + WIFI_MAX_EAP_PASSWORD_LENGTH + 1 + WIFI_MAX_EAP_USERNAME_LENGTH + 1 + WIFI_PHASE_TWO_TYPE_LENGTH;
export const WIFI_CONNECTION_DETAILS_LENGTH =
  1 + // encryptionType
  WIFI_MAX_SSID_LENGTH + 1 + // ssid
  EAP_WIFI_CONNECTION_CREDENTIALS_LENGTH; // union: biggest branch

export enum WiFiScanStatus {
  NOT_STARTED = 0,
  RUNNING = 1,
  COMPLETED = 2,
  FAILED = 3
}

export enum WiFiStatus {
  DISCONNECTED = 0,
  CONNECTED = 1,
  CONNECTED_NO_IP = 2,
  WRONG_PASSWORD = 3,
  NO_AP_FOUND = 4,
  CONNECTION_FAILED = 5,
  UNKNOWN = 255
}

export enum WiFiEncryptionType {
  WIFI_AUTH_OPEN = 0,
  WIFI_AUTH_WEP,
  WIFI_AUTH_WPA_PSK,
  WIFI_AUTH_WPA2_PSK,
  WIFI_AUTH_WPA_WPA2_PSK,
  WIFI_AUTH_ENTERPRISE,
  WIFI_AUTH_WPA2_ENTERPRISE = 5,
  WIFI_AUTH_WPA3_PSK,
  WIFI_AUTH_WPA2_WPA3_PSK,
  WIFI_AUTH_WAPI_PSK,
  WIFI_AUTH_WPA3_ENT_192
}

export const ENTERPRISE_ENCRYPTION_TYPES: WiFiEncryptionType[] = [
  WiFiEncryptionType.WIFI_AUTH_WPA2_ENTERPRISE,
  WiFiEncryptionType.WIFI_AUTH_WPA3_ENT_192
];

export enum WiFiPhaseTwoType {
  ESP_EAP_TTLS_PHASE2_EAP,
  ESP_EAP_TTLS_PHASE2_MSCHAPV2,
  ESP_EAP_TTLS_PHASE2_MSCHAP,
  ESP_EAP_TTLS_PHASE2_PAP,
  ESP_EAP_TTLS_PHASE2_CHAP
}

export interface WiFiNetwork {
  ssid: string;
  encryptionType: WiFiEncryptionType;
}

export interface WiFiDetails {
  ssid: string;
  mac: string;
  ip: number;       // IPv4 address as a 32-bit integer
  gateway: number;
  subnet: number;
  dns: number;
}

export interface WiFiConnectionDetails {
  encryptionType: WiFiEncryptionType;
  ssid: string;
  credentials: SimpleWiFiConnectionCredentials | EAPWiFiConnectionCredentials;
}

export interface SimpleWiFiConnectionCredentials {
  password: string;
}

export interface EAPWiFiConnectionCredentials {
  identity: string;
  username: string;
  password: string;
  phase2Type: WiFiPhaseTwoType;
}

export function isEnterprise(encryptionType: WiFiEncryptionType): boolean {
  return ENTERPRISE_ENCRYPTION_TYPES.includes(encryptionType);
}

