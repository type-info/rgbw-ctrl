import {DeviceState} from "./device-state.model";

const BASE_URL = "/rest";

export async function getState(): Promise<DeviceState> {
    const response = await fetch(`${BASE_URL}/state`);
    if (!response.ok) throw new Error("Failed to fetch device state");
    return response.json();
}

export async function setColor(r: number, g: number, b: number, w: number): Promise<void> {
    const params = new URLSearchParams({ r: r.toString(), g: g.toString(), b: b.toString(), w: w.toString() });
    const response = await fetch(`${BASE_URL}/color?${params.toString()}`);
    if (!response.ok) throw new Error("Failed to set color");
}

export async function setBluetoothState(enabled: boolean): Promise<void> {
    const state = enabled ? "on" : "off";
    const response = await fetch(`${BASE_URL}/bluetooth?state=${state}`);
    if (!response.ok) throw new Error("Failed to change Bluetooth state");
}

export async function restartSystem(): Promise<void> {
    const response = await fetch(`${BASE_URL}/system/restart`);
    if (!response.ok) throw new Error("Failed to restart device");
}

export async function resetSystem(): Promise<void> {
    const response = await fetch(`${BASE_URL}/system/reset`);
    if (!response.ok) throw new Error("Failed to reset device");
}
