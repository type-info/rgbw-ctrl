import {AlexaIntegrationSettings} from "./alexa-integration-settings.model";
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
} from "./encode.utils";
import {} from "./decode.utils";
import {WebSocketMessageType} from "./websocket.message";
import {WiFiConnectionDetails} from "./wifi.model";

const DEVICE_NAME_MAX_LENGTH = 31;
const RECONNECT_INTERVAL = 5000; // ms

export const webSocketHandlers = new Map<WebSocketMessageType, (data: ArrayBuffer) => void>();

let socket: WebSocket;
let reconnecting = false;

export function initWebSocket(url: string) {
  socket = new WebSocket(url);
  socket.binaryType = "arraybuffer";

  socket.onopen = () => {
    console.info("WebSocket connected");
    reconnecting = false;
  };

  socket.onmessage = (event: MessageEvent<ArrayBuffer>) => handleMessage(event.data);

  socket.onerror = (err) => {
    console.error("WebSocket error", err);
    tryReconnect(url);
  };

  socket.onclose = () => {
    console.warn("WebSocket closed");
    tryReconnect(url);
  };
}

function tryReconnect(url: string) {
  if (reconnecting) return;
  reconnecting = true;

  setTimeout(() => {
    console.info("Reconnecting WebSocket...");
    initWebSocket(url);
  }, RECONNECT_INTERVAL);
}

function send(message: Uint8Array) {
  if (socket?.readyState === WebSocket.OPEN) {
    socket.send(message);
  } else {
    console.warn("WebSocket is not open. Message not sent.");
  }
}

function handleMessage(message: ArrayBuffer) {
  const data = new Uint8Array(message);
  const type = data[0] as WebSocketMessageType;

  const handler = webSocketHandlers.get(type);
  if (handler) {
    handler(message);
  } else {
    console.warn("Unhandled message type", type);
  }
}

export function sendColorMessage(r: number, g: number, b: number, w: number): void {
  const message = encodeColorMessage([r, g, b, w]);
  send(message);
}

export function sendDeviceName(name: string): void {
  const buffer = encodeDeviceNameMessage(name, DEVICE_NAME_MAX_LENGTH);
  send(buffer);
}

export function sendHttpCredentials(username: string, password: string): void {
  const buffer = encodeHttpCredentialsMessage({username, password});
  send(buffer);
}

export function sendWiFiConnectionDetails(details: WiFiConnectionDetails): void {
  const buffer = encodeWiFiConnectionDetailsMessage(details);
  send(buffer);
}

export function sendBleStatus(status: number): void {
  const buffer = encodeBleStatusMessage(status);
  send(buffer);
}

export function sendAlexaIntegrationSettings(settings: AlexaIntegrationSettings): void {
  const buffer = encodeAlexaIntegrationSettingsMessage(settings);
  send(buffer);
}

export function sendHeapPing(): void {
  const buffer = encodeHeapMessage();
  send(buffer);
}

export function sendWiFiScanRequest(): void {
  const buffer = encodeWiFiScanStatusMessage();
  send(buffer);
}

export function sendOtaProgressRequest(): void {
  const buffer = encodeOtaProgressMessage();
  send(buffer);
}
