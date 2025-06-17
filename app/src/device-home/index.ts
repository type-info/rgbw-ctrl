import { WebSocketHandler } from '../web-socket.handler';

const ws = new WebSocketHandler(`ws://${location.host}/ws`);

const bluetoothButton = document.getElementById('bluetooth-toggle')!;
let bluetoothOn = false;

bluetoothButton.addEventListener('click', () => {
  bluetoothOn = !bluetoothOn;
  bluetoothButton.textContent = `Bluetooth: ${bluetoothOn ? 'ON' : 'OFF'}`;
  ws.sendBleStatus(bluetoothOn ? 1 : 0);
});

const sliders = document.querySelectorAll<HTMLInputElement>('label.slider-label input[type="range"]');

function getColorValues(): [number, number, number, number] {
  return Array.from(sliders).map(s => parseInt(s.value, 10)) as [number, number, number, number];
}

sliders.forEach(slider => {
  slider.addEventListener('input', () => {
    ws.sendColorMessage(...getColorValues());
  });
});
