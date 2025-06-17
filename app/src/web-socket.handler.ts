import {AlexaIntegrationSettings} from "./app/alexa-integration-settings.model";
import {
  encodeAlexaIntegrationSettingsMessage,
  encodeBleStatusMessage,
  encodeColorMessage,
  encodeDeviceNameMessage,
  encodeHeapMessage,
  encodeHttpCredentialsMessage,
  encodeOtaProgressMessage,
  encodeWiFiConnectionDetailsMessage,
  encodeWiFiScanStatusMessage,
} from "./app/encode.utils";
import {
  WebSocketMessageType
} from "./app/websocket.message";
import {WiFiConnectionDetails} from "./app/wifi.model";

const DEVICE_NAME_MAX_LENGTH = 31;

export class WebSocketHandler {
  private socket: WebSocket;

  constructor(private url: string) {
    this.socket = new WebSocket(url);
    this.socket.binaryType = "arraybuffer";
    this.socket.onmessage = (event: MessageEvent<ArrayBuffer>) => this.handleMessage(event.data);
  }

  private send(message: Uint8Array) {
    this.socket.send(message);
  }

  private handleMessage(message: ArrayBuffer) {
    const data = new Uint8Array(message);
    const type = data[0] as WebSocketMessageType;

    switch (type) {
      case WebSocketMessageType.ON_HEAP:
        console.debug("Received HEAP message");
        break;
      case WebSocketMessageType.ON_WIFI_SCAN_STATUS:
        console.debug("Received WIFI_SCAN_STATUS message");
        break;
      case WebSocketMessageType.ON_OTA_PROGRESS:
        console.debug("Received OTA_PROGRESS message");
        break;
      default:
        console.warn("Unknown message type", type);
        break;
    }
  }

  public sendColorMessage(r: number, g: number, b: number, w: number): void {
    const message = encodeColorMessage([r, g, b, w]);
    this.send(message);
  }

  public sendDeviceName(name: string): void {
    const buffer = encodeDeviceNameMessage(name, DEVICE_NAME_MAX_LENGTH);
    this.send(buffer);
  }

  public sendHttpCredentials(username: string, password: string): void {
    const buffer = encodeHttpCredentialsMessage({username, password});
    this.send(buffer);
  }

  public sendWiFiConnectionDetails(details: WiFiConnectionDetails): void {
    const buffer = encodeWiFiConnectionDetailsMessage(details);
    this.send(buffer);
  }

  public sendBleStatus(status: number): void {
    const buffer = encodeBleStatusMessage(status);
    this.send(buffer);
  }

  public sendAlexaIntegrationSettings(settings: AlexaIntegrationSettings): void {
    const buffer = encodeAlexaIntegrationSettingsMessage(settings);
    this.send(buffer);
  }

  public sendHeapPing(): void {
    const buffer = encodeHeapMessage();
    this.send(buffer);
  }

  public sendWiFiScanRequest(): void {
    const buffer = encodeWiFiScanStatusMessage();
    this.send(buffer);
  }

  public sendOtaProgressRequest(): void {
    const buffer = encodeOtaProgressMessage();
    this.send(buffer);
  }
}
