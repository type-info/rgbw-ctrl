import {getState, restartSystem, resetSystem, setBluetoothState} from "./rest-api.ts";
import {
    initWebSocket,
    sendColorMessage,
    webSocketHandlers,
    sendBleStatus
} from "../../app/src/app/web-socket.handler.ts";
import {from, fromEvent, map, mergeMap, tap, throttleTime} from "rxjs";
import {
    decodeWebSocketOnColorMessage,
    decodeWebSocketOnBleStatusMessage,
    decodeDeviceNameMessage
} from "../../app/src/app/decode.utils.ts"
import {WebSocketMessageType} from "../../app/src/app/websocket.message.ts"
import {BleStatus} from "../../app/src/app/ble.model.ts"

const sliders = Array.from(document.querySelectorAll<HTMLInputElement>('label.slider-label input[type="range"]'));
const resetButton = document.querySelector<HTMLButtonElement>("#restart-btn")!;
const restartButton = document.querySelector<HTMLButtonElement>("#reset-btn")!;
const bluetoothButton = document.querySelector<HTMLButtonElement>("#bluetooth-toggle")!;

loadDeviceState();
initializeSliders();
initWebSocket(`ws://${location.host}/ws`);

resetButton.addEventListener("click", async (e) => {
    e.preventDefault();
    if (confirm("Restart the device?")) {
        await restartSystem();
    }
});

restartButton.addEventListener("click", async (e) => {
    e.preventDefault();
    if (confirm("Factory reset the device?")) {
        await resetSystem();
    }
});

bluetoothButton.addEventListener("click", async () => {
    const current = bluetoothButton.dataset.enabled === "true";
    bluetoothButton.disabled = true;
    sendBleStatus(current ? BleStatus.OFF : BleStatus.ADVERTISING);
});

webSocketHandlers.set(WebSocketMessageType.ON_BLE_STATUS, (message: ArrayBuffer) => {
    const {status} = decodeWebSocketOnBleStatusMessage(message);
    updateBluetoothButton(status);
    bluetoothButton.disabled = false;
});

function updateBluetoothButton(status: BleStatus): void {
    const statusString = status === BleStatus.OFF
        ? "OFF"
        : status === BleStatus.ADVERTISING
            ? "ADVERTISING"
            : "CONNECTED";
    bluetoothButton.dataset.enabled = status !== BleStatus.OFF ? "true" : "false";
    bluetoothButton.textContent = `Bluetooth: ${statusString}`;
    bluetoothButton.classList.toggle("active", status !== BleStatus.OFF);
    bluetoothButton.disabled = false;
}

webSocketHandlers.set(WebSocketMessageType.ON_COLOR, (message: ArrayBuffer) => {
    const {values} = decodeWebSocketOnColorMessage(message);
    values.forEach(({on, value}, index) => {
        const slider = sliders[index];
        slider.value = value.toString();
        updateSliderVisual(slider, getComputedStyle(slider.closest(".slider-label")!).getPropertyValue("--color").trim());
    })
});

webSocketHandlers.set(WebSocketMessageType.ON_DEVICE_NAME, (message: ArrayBuffer) => {
    const {deviceName} = decodeDeviceNameMessage(message);
    updateText("device-name", deviceName);
})

function updateSliderVisual(slider: HTMLInputElement, color: string) {
    const [_, r, g, b] = color.match(/rgb\((\d+),\s*(\d+),\s*(\d+)/)!;
    const value = parseInt(slider.value, 10);
    const percentage = Math.round((value / 255) * 100);
    slider.style.background = `linear-gradient(to right, ${color} 0%, ${color} ${percentage}%, rgba(${r}, ${g}, ${b}, 0.3) ${percentage}%, rgba(${r}, ${g}, ${b}, 0.3) 100%)`;
    const labelSpan = slider.parentElement?.querySelector("span");
    if (labelSpan) labelSpan.textContent = `${percentage}%`;
}

function initializeSliders(): void {
    from(sliders).pipe(
        mergeMap(slider => fromEvent(slider, 'input')),
        tap(event => {
            const slider = event.target as HTMLInputElement;
            const label = slider.closest('.slider-label');
            const color = getComputedStyle(label!).getPropertyValue('--color').trim();
            updateSliderVisual(slider, color);
        }),
        throttleTime(300, undefined, {leading: true, trailing: true}),
        map(() => getColorValues()),
    ).subscribe(values => sendColorMessage(...values));
}

function getColorValues(): [number, number, number, number] {
    return sliders.map(s => parseInt(s.value, 10)) as [number, number, number, number];
}

function updateText(id: string, value: string): void {
    const el = document.getElementById(id);
    if (el) el.textContent = value;
}

async function loadDeviceState(): Promise<void> {
    try {
        const state = await getState();

        updateText("device-name", state.deviceName);
        updateText("firmware-version", state.firmwareVersion);
        updateText("heap", `${state.heap}`);

        updateText("wifi-status", state.wifi.status);
        updateText("wifi-ssid", state.wifi.details.ssid);
        updateText("wifi-mac", state.wifi.details.mac);
        updateText("wifi-ip", state.wifi.details.ip);
        updateText("wifi-gateway", state.wifi.details.gateway);
        updateText("wifi-subnet", state.wifi.details.subnet);
        updateText("wifi-dns", state.wifi.details.dns);

        updateText("alexa-mode", state.alexa.mode);
        updateText("alexa-names", state.alexa.names.join(", "));

        updateText("ota-status", state.ota.state);
    } catch (error) {
        console.error("Failed to load device state", error);
    }
}
