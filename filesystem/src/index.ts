import {getState, restartSystem, resetSystem, setBluetoothState} from "./rest-api.ts";
import {initWebSocket, sendColorMessage, webSocketHandlers} from "../../app/src/app/web-socket.handler.ts";
import {from, fromEvent, map, mergeMap, tap, throttleTime} from "rxjs";
import {decodeWebSocketOnColorMessage} from "../../app/src/app/decode.utils.ts"
import {WebSocketMessageType} from "../../app/src/app/websocket.message.ts"

const sliders = document.querySelectorAll<HTMLInputElement>('label.slider-label input[type="range"]');

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
    const sliders = document.querySelectorAll<HTMLInputElement>('label.slider-label input[type="range"]');
    return Array.from(sliders).map(s => parseInt(s.value, 10)) as [number, number, number, number];
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

        updateText("ble-status", state.ble.status);
        updateText("ota-status", state.ota.state);
    } catch (error) {
        console.error("Failed to load device state", error);
    }
}

function setupBluetoothToggle(): void {
    const button = document.getElementById("bluetooth-toggle") as HTMLButtonElement;
    button.addEventListener("click", async () => {
        const current = button.dataset.enabled === "true";
        const next = !current;
        button.disabled = true;
        try {
            await setBluetoothState(next);
            button.dataset.enabled = String(next);
            button.textContent = next ? "Bluetooth: ON" : "Bluetooth: OFF";
            button.classList.toggle("active", next);
        } catch (err) {
            alert("Failed to change Bluetooth state");
        } finally {
            button.disabled = false;
        }
    });
}

function setupSystemButtons(): void {
    document.getElementById("restart-btn")?.addEventListener("click", async (e) => {
        e.preventDefault();
        if (confirm("Restart the device?")) {
            await restartSystem();
        }
    });

    document.getElementById("reset-btn")?.addEventListener("click", async (e) => {
        e.preventDefault();
        if (confirm("Factory reset the device?")) {
            await resetSystem();
        }
    });
}

loadDeviceState();
initializeSliders();
setupBluetoothToggle();
setupSystemButtons();
initWebSocket(`ws://${location.host}/ws`);

webSocketHandlers.set(WebSocketMessageType.ON_COLOR, (message: ArrayBuffer) => {
    const values = decodeWebSocketOnColorMessage(message).values;
    values.forEach(({on, value}, index) => {
        const slider = sliders[index];
        slider.value = value.toString();
        updateSliderVisual(slider, getComputedStyle(slider.closest(".slider-label")!).getPropertyValue("--color").trim());
    })
});