import {sendBleStatus, sendColorMessage, webSocketHandlers, initWebSocket} from '../../app/src/app/web-socket.handler.ts';
import {WebSocketMessageType} from '../../app/src/app/websocket.message.ts';
import {decodeWebSocketOnBleStatusMessage} from '../../app/src/app/decode.utils.ts';
import {BleStatus} from '../../app/src/app/ble.model.ts';

const bluetoothButton = document.getElementById('bluetooth-toggle')!;
bluetoothButton.addEventListener('click', () => {
    bluetoothButton.setAttribute('disabled', 'true');
    sendBleStatus(bluetoothButton.textContent?.includes('ON') ? BleStatus.OFF : BleStatus.ADVERTISING);
});

webSocketHandlers.set(WebSocketMessageType.ON_BLE_STATUS, (data: ArrayBuffer) => {
    const msg = decodeWebSocketOnBleStatusMessage(data);
    bluetoothButton.textContent = `Bluetooth: ${msg.status !== BleStatus.OFF ? 'ON' : 'OFF'}`;
    bluetoothButton.setAttribute('disabled', '');
});

const sliders = document.querySelectorAll<HTMLInputElement>('label.slider-label input[type="range"]');

function getColorValues(): [number, number, number, number] {
    return Array.from(sliders).map(s => parseInt(s.value, 10)) as [number, number, number, number];
}

sliders.forEach(slider => {
    slider.addEventListener('input', () => {
        sendColorMessage(...getColorValues());
    });
});

initWebSocket(`ws://${location.host}/ws`);