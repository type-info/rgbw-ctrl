import {Component, OnDestroy} from '@angular/core';
import {MatSnackBar} from '@angular/material/snack-bar';
import {CommonModule, NgOptimizedImage} from '@angular/common';
import {MatButtonModule} from '@angular/material/button';
import {MatIconModule} from '@angular/material/icon';
import {MatCardModule} from '@angular/material/card';
import {FormControl, FormGroup, ReactiveFormsModule, Validators} from '@angular/forms';
import {
  isEnterprise,
  WiFiConnectionDetails,
  WiFiDetails,
  WiFiEncryptionType,
  WiFiNetwork,
  WiFiScanStatus,
  WiFiStatus
} from '../wifi.model';
import {MatListModule} from '@angular/material/list';
import {MatLineModule} from '@angular/material/core';
import {MatFormFieldModule} from '@angular/material/form-field';
import {MatExpansionModule} from '@angular/material/expansion';
import {MatInputModule} from '@angular/material/input';
import {
  decodeAlexaIntegrationSettings,
  decodeCString,
  decodeOtaCredentials,
  decodeWiFiDetails,
  decodeWiFiScanResult,
  decodeWiFiScanStatus,
  decodeWiFiStatus
} from '../decode.utils';
import {
  encodeAlexaIntegrationSettings,
  encodeOtaCredentials,
  encodeWiFiConnectionDetails,
  textEncoder
} from '../encode.utils';
import {MatButtonToggleModule} from '@angular/material/button-toggle';
import {NumberToIpPipe} from '../number-to-ip.pipe';
import {MatDialog, MatDialogModule} from '@angular/material/dialog';
import {LoadingComponent} from '../loading/loading.component';
import {MatTooltipModule} from '@angular/material/tooltip';
import {MatProgressSpinnerModule} from '@angular/material/progress-spinner';
import {EditDeviceNameComponentDialog} from './edit-device-name-dialog/edit-device-name-component-dialog.component';
import {asyncScheduler, firstValueFrom, Subscription, throttleTime} from 'rxjs';
import {ALEXA_MAX_DEVICE_NAME_LENGTH, AlexaIntegrationMode} from '../alexa-integration-settings.model';
import {MatChipsModule} from '@angular/material/chips';
import {
  EnterpriseWiFiConnectDialogComponent
} from './enterprise-wi-fi-connect-dialog/enterprise-wi-fi-connect-dialog.component';
import {SimpleWiFiConnectDialogComponent} from './simple-wi-fi-connect-dialog/simple-wi-fi-connect-dialog.component';
import {CustomWiFiConnectDialogComponent} from './custom-wi-fi-connect-dialog/custom-wi-fi-connect-dialog.component';
import {MAX_OTA_PASSWORD_LENGTH, MAX_OTA_USERNAME_LENGTH} from '../ota.model';
import {KilobytesPipe} from '../kb.pipe';
import {MatSliderModule} from '@angular/material/slider';
import {ConfirmAlexaRestart} from '../yes-no-dialog/confirm-alexa-restart.component';

const BLE_NAME = "rgbw-ctrl";

const DEVICE_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890ac";
const DEVICE_RESTART_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000";
const DEVICE_NAME_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001";
const FIRMWARE_VERSION_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002";
const OTA_CREDENTIALS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003";
const DEVICE_HEAP_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004";

const WIFI_SERVICE = "12345678-1234-1234-1234-1234567890ab";
const WIFI_DETAILS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";
const WIFI_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006";
const WIFI_SCAN_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";
const WIFI_SCAN_RESULT_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008";

const ALEXA_SERVICE = "12345678-1234-1234-1234-1234567890ba";
const ALEXA_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009";
const ALEXA_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a";

@Component({
  selector: 'app-rgbw-ctrl',
  standalone: true,
  imports: [
    CommonModule,
    MatButtonModule,
    MatIconModule,
    MatCardModule,
    MatListModule,
    MatLineModule,
    MatFormFieldModule,
    MatExpansionModule,
    MatInputModule,
    MatDialogModule,
    MatButtonToggleModule,
    MatTooltipModule,
    MatProgressSpinnerModule,
    MatSliderModule,
    NumberToIpPipe,
    ReactiveFormsModule,
    MatChipsModule,
    KilobytesPipe,
    NgOptimizedImage
  ],
  templateUrl: './rgbw-ctrl.component.html',
  styleUrls: ['./rgbw-ctrl.component.scss']
})
export class RgbwCtrlComponent implements OnDestroy {

  readonly WiFiStatus = WiFiStatus;
  readonly AlexaIntegrationMode = AlexaIntegrationMode;
  readonly ALEXA_MAX_DEVICE_NAME_LENGTH = ALEXA_MAX_DEVICE_NAME_LENGTH - 1;
  readonly MAX_OTA_USERNAME_LENGTH = MAX_OTA_USERNAME_LENGTH;
  readonly MAX_OTA_PASSWORD_LENGTH = MAX_OTA_PASSWORD_LENGTH;

  private colorSubscription: Subscription;
  alexaIntegrationModes = [
    {value: AlexaIntegrationMode.OFF, label: '❌', title: 'Off'},
    {value: AlexaIntegrationMode.RGBW_DEVICE, label: '🔴🟢🔵⚪', title: 'RGBW'},
    {value: AlexaIntegrationMode.RGB_DEVICE, label: '🔴🟢🔵💡', title: 'RGB'},
    {value: AlexaIntegrationMode.MULTI_DEVICE, label: '💡💡💡💡', title: 'Multi Device'}
  ];

  private server: BluetoothRemoteGATTServer | null = null;

  private deviceNameService: BluetoothRemoteGATTService | null = null;
  private deviceRestartCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private deviceNameCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private firmwareVersionCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private otaCredentialsCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private deviceHeapCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  private wifiService: BluetoothRemoteGATTService | null = null;
  private wifiDetailsCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private wifiStatusCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private wifiScanStatusCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private wifiScanResultCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  private alexaService: BluetoothRemoteGATTService | null = null;
  private alexaCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private alexaColorCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  initialized: boolean = false;
  loadingAlexa: boolean = false;
  loadingOtaCredentials = false;
  readingAlexaColor = false;

  firmwareVersion: string | null = null;
  deviceName: string | null = null;
  deviceHeap: number = 0;

  wifiStatus: WiFiStatus = WiFiStatus.UNKNOWN;
  wifiScanStatus: WiFiScanStatus = WiFiScanStatus.NOT_STARTED;
  wifiScanResult: WiFiNetwork[] = [];
  wifiDetails: WiFiDetails | null = null;

  alexaIntegrationForm = new FormGroup({
    integrationMode: new FormControl<AlexaIntegrationMode>(0, {nonNullable: true, validators: [Validators.required]}),
    rDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
    gDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
    bDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
    wDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
  });

  colorForm = new FormGroup({
    r: new FormControl<number>(0, {
      nonNullable: true
    }),
    g: new FormControl<number>(0, {
      nonNullable: true
    }),
    b: new FormControl<number>(0, {
      nonNullable: true
    }),
    w: new FormControl<number>(0, {
      nonNullable: true
    }),
  });

  otaCredentialsForm = new FormGroup({
    username: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(MAX_OTA_USERNAME_LENGTH)]
    }),
    password: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(MAX_OTA_PASSWORD_LENGTH)]
    }),
  });

  get scanning() {
    return this.wifiScanStatus === WiFiScanStatus.RUNNING;
  }

  get connected(): boolean {
    return this.server?.connected || false;
  }

  constructor(
    private matDialog: MatDialog,
    private snackBar: MatSnackBar
  ) {
    const perceptualMap = (v: number): number => {
      const normalized = Math.min(Math.max(v / 100, 0), 1);
      return Math.round(Math.pow(Math.sin(normalized * (Math.PI / 2)), 2) * 255);
    };

    this.colorSubscription = this.colorForm.valueChanges.pipe(
      throttleTime(200, asyncScheduler, {
        leading: true,
        trailing: true
      })
    ).subscribe(value => {
      if (this.alexaColorCharacteristic) {
        const color = new Uint8Array(4);
        color[0] = perceptualMap(value.r ?? 0);
        color[1] = perceptualMap(value.g ?? 0);
        color[2] = perceptualMap(value.b ?? 0);
        color[3] = perceptualMap(value.w ?? 0);
        this.alexaColorCharacteristic.writeValue(color)
          .catch(console.error);
      }
    });

  }

  ngOnDestroy() {
    this.colorSubscription.unsubscribe();
    this.disconnect();
  }

  disconnect() {
    this.initialized = false;
    this.server?.disconnect();
    this.server = null;

    // Clear GATT services and characteristics
    this.deviceNameService = null;
    this.deviceRestartCharacteristic = null;
    this.deviceNameCharacteristic = null;
    this.firmwareVersionCharacteristic = null;

    this.wifiService = null;
    this.wifiDetailsCharacteristic = null;
    this.wifiStatusCharacteristic = null;
    this.wifiScanStatusCharacteristic = null;
    this.wifiScanResultCharacteristic = null;

    this.alexaService = null;
    this.alexaCharacteristic = null;
    this.alexaColorCharacteristic = null;

    // Clear local UI state
    this.deviceName = null;
    this.firmwareVersion = null;
    this.wifiDetails = null;
    this.wifiScanStatus = WiFiScanStatus.NOT_STARTED;
    this.wifiStatus = WiFiStatus.UNKNOWN;
    this.wifiScanResult = [];

    this.loadingAlexa = false;
    this.loadingOtaCredentials = false;
    this.readingAlexaColor = false;

    this.alexaIntegrationForm.reset();
  }

  async connectBleDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true})
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{namePrefix: BLE_NAME}],
        optionalServices: [WIFI_SERVICE, DEVICE_DETAILS_SERVICE, ALEXA_SERVICE]
      });

      if (!device.gatt) {
        this.snackBar.open('Device does not support GATT', 'Close', {duration: 3000});
        return;
      }

      this.server = await device.gatt.connect();

      await this.initBleWiFiServices(this.server);
      await this.initBleDeviceNameServices(this.server);
      await this.initBleAlexaIntegrationServices(this.server);
      device.addEventListener('gattserverdisconnected', () => {
        this.disconnect();
        this.snackBar.open('Device disconnected', 'Reconnect', {duration: 3000})
          .onAction().subscribe(() => this.connectBleDevice());
      });

      await this.readDeviceName();
      await this.readFirmwareVersion();
      await this.readOtaCredentials();
      await this.readWiFiStatus();
      await this.readWiFiDetails();
      await this.readWiFiScanStatus();
      await this.readWiFiScanResult();
      await this.readAlexaIntegration();
      await this.readAlexaColor();
      this.snackBar.open('Device connected', 'Close', {duration: 3000});
      this.initialized = true;
      if (this.wifiScanResult.length === 0) {
        await this.startWifiScan();
      }
    } catch (error) {
      console.error(error);
      this.snackBar.open('Failed to connect to device', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  async startWifiScan() {
    if (!this.wifiScanStatusCharacteristic) return;
    await this.wifiScanStatusCharacteristic.writeValue(new Uint8Array([0]));
  }

  async restartDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.deviceRestartCharacteristic!.writeValue(textEncoder.encode("RESTART_NOW"));
    } catch (e) {
      console.error('Failed to restart device:', e);
      this.snackBar.open('Failed to restart device', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  async connectToNetwork(network: WiFiNetwork) {
    let details: Uint8Array;
    if (network.encryptionType === WiFiEncryptionType.WIFI_AUTH_OPEN) {
      const value: WiFiConnectionDetails = {
        encryptionType: network.encryptionType,
        ssid: network.ssid,
        credentials: {password: ''}
      };
      details = encodeWiFiConnectionDetails(value);
    } else if (isEnterprise(network.encryptionType)) {
      let value = await firstValueFrom(this.matDialog.open(EnterpriseWiFiConnectDialogComponent, {data: network}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    } else {
      let value = await firstValueFrom(this.matDialog.open(SimpleWiFiConnectDialogComponent, {data: network}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    }
    await this.sendWifiConfig(details);
  }

  async connectToCustomNetwork() {
    const data: WiFiConnectionDetails = await firstValueFrom(this.matDialog.open(CustomWiFiConnectDialogComponent).afterClosed());
    if (!data) return;
    let details: Uint8Array;
    if (isEnterprise(data.encryptionType)) {
      let value = await firstValueFrom(this.matDialog.open(EnterpriseWiFiConnectDialogComponent, {data}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    } else {
      let value = await firstValueFrom(this.matDialog.open(SimpleWiFiConnectDialogComponent, {data}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    }
    await this.sendWifiConfig(details);
  }

  async editDeviceName() {
    const deviceName = await firstValueFrom(this.matDialog.open(EditDeviceNameComponentDialog, {data: this.deviceName}).afterClosed())
    if (deviceName) {
      const encodedName = textEncoder.encode(deviceName);
      await this.deviceNameCharacteristic!.writeValue(encodedName);
      this.snackBar.open('Device name updated', 'Close', {duration: 3000});
    }
  }

  async loadAlexaIntegrationSettings() {
    this.loadingAlexa = true;
    await this.readAlexaIntegration();
    this.loadingAlexa = false;
  }

  async applyAlexaIntegrationSettings() {
    if (this.alexaIntegrationForm.invalid || !this.connected) {
      return;
    }
    const shouldRestart = await firstValueFrom(this.matDialog.open(ConfirmAlexaRestart, {disableClose: true}).afterClosed());
    // Encode the form value and write via BLE
    const settings = this.alexaIntegrationForm.getRawValue();
    const payload = encodeAlexaIntegrationSettings(settings);
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.alexaCharacteristic!.writeValue(payload);
      this.snackBar.open('Alexa settings updated', 'Close', {duration: 2500});
    } catch (e) {
      console.log('Failed to update Alexa settings:', e);
      this.snackBar.open('Failed to update Alexa settings', 'Close', {duration: 3000});
    } finally {
      if (shouldRestart) {
        await this.restartDevice();
      }
      loading.close();
    }
  }

  async loadOtaCredentials() {
    this.loadingOtaCredentials = true;
    await this.readOtaCredentials();
    this.loadingOtaCredentials = false;
  }

  async applyOtaCredentials() {
    if (this.otaCredentialsForm.invalid || !this.connected) {
      return;
    }
    const credentials = this.otaCredentialsForm.getRawValue();
    const payload = encodeOtaCredentials(credentials);
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.otaCredentialsCharacteristic!.writeValue(payload);
      this.snackBar.open('OTA credentials updated', 'Close', {duration: 3000});
    } catch (e) {
      console.error('Failed to update OTA credentials:', e);
      this.snackBar.open('Failed to update OTA credentials', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  private async initBleWiFiServices(server: BluetoothRemoteGATTServer) {
    this.wifiService = await server.getPrimaryService(WIFI_SERVICE);

    this.wifiDetailsCharacteristic = await this.wifiService.getCharacteristic(WIFI_DETAILS_CHARACTERISTIC);
    this.wifiDetailsCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiDetailsChanged(ev.target.value));
    await this.wifiDetailsCharacteristic.startNotifications();

    this.wifiStatusCharacteristic = await this.wifiService.getCharacteristic(WIFI_STATUS_CHARACTERISTIC);
    this.wifiStatusCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiStatusChanged(ev.target.value));
    await this.wifiStatusCharacteristic.startNotifications();

    this.wifiScanStatusCharacteristic = await this.wifiService.getCharacteristic(WIFI_SCAN_STATUS_CHARACTERISTIC);
    this.wifiScanStatusCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiScanStatusChanged(ev.target.value));
    await this.wifiScanStatusCharacteristic.startNotifications();

    this.wifiScanResultCharacteristic = await this.wifiService.getCharacteristic(WIFI_SCAN_RESULT_CHARACTERISTIC);
    this.wifiScanResultCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiScanResultChanged(ev.target.value));
    await this.wifiScanResultCharacteristic.startNotifications();
  }

  private async initBleDeviceNameServices(server: BluetoothRemoteGATTServer) {
    this.deviceNameService = await server.getPrimaryService(DEVICE_DETAILS_SERVICE);

    this.deviceRestartCharacteristic = await this.deviceNameService.getCharacteristic(DEVICE_RESTART_CHARACTERISTIC);
    this.firmwareVersionCharacteristic = await this.deviceNameService.getCharacteristic(FIRMWARE_VERSION_CHARACTERISTIC);
    this.otaCredentialsCharacteristic = await this.deviceNameService.getCharacteristic(OTA_CREDENTIALS_CHARACTERISTIC);

    this.deviceNameCharacteristic = await this.deviceNameService.getCharacteristic(DEVICE_NAME_CHARACTERISTIC);
    this.deviceNameCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.deviceNameChanged(ev.target.value));
    await this.deviceNameCharacteristic.startNotifications();


    this.deviceHeapCharacteristic = await this.deviceNameService.getCharacteristic(DEVICE_HEAP_CHARACTERISTIC);
    this.deviceHeapCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.deviceHeapChanged(ev.target.value));
    await this.deviceHeapCharacteristic.startNotifications();
  }

  private async initBleAlexaIntegrationServices(server: BluetoothRemoteGATTServer) {
    this.alexaService = await server.getPrimaryService(ALEXA_SERVICE);
    this.alexaCharacteristic = await this.alexaService.getCharacteristic(ALEXA_CHARACTERISTIC);
    this.alexaColorCharacteristic = await this.alexaService.getCharacteristic(ALEXA_COLOR_CHARACTERISTIC);
    this.alexaColorCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.alexaColorChanged(ev.target.value));
    await this.alexaColorCharacteristic.startNotifications();
  }

  private wifiStatusChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiStatus = decodeWiFiStatus(buffer);
  }

  private wifiDetailsChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiDetails = decodeWiFiDetails(buffer);
  }

  private wifiScanStatusChanged(view: DataView) {
    this.wifiScanStatus = decodeWiFiScanStatus(new Uint8Array(view.buffer));
    if (this.wifiScanStatus === WiFiScanStatus.FAILED) {
      this.snackBar.open(`WiFi scan failed!`, 'Close', {duration: 3000});
    }
  }

  private wifiScanResultChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiScanResult = decodeWiFiScanResult(buffer);
  }

  private deviceNameChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.deviceName = decodeCString(buffer);
  }

  private deviceHeapChanged(view: DataView) {
    this.deviceHeap = view.getUint32(0, true);
  }

  private alexaColorChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    const inversePerceptualMap = (pwm: number): number => {
      if (pwm <= 0) return 0;
      if (pwm >= 255) return 100;
      const normalized = (2 / Math.PI) * Math.asin(Math.sqrt(pwm / 255));
      return Math.round(normalized * 100);
    };

    this.colorForm.setValue({
      r: inversePerceptualMap(buffer[0]),
      g: inversePerceptualMap(buffer[1]),
      b: inversePerceptualMap(buffer[2]),
      w: inversePerceptualMap(buffer[3]),
    }, {emitEvent: false});
  }

  private otaCredentialsChanged(view: DataView) {
    const credentials = decodeOtaCredentials(new Uint8Array(view.buffer));
    this.otaCredentialsForm.reset(credentials);
  }

  private async readOtaCredentials() {
    const value = await this.otaCredentialsCharacteristic!.readValue();
    this.otaCredentialsChanged(value);
  }

  private async readDeviceName() {
    const view = await this.deviceNameCharacteristic!.readValue();
    this.deviceNameChanged(view);
  }

  private async readFirmwareVersion() {
    const view = await this.firmwareVersionCharacteristic!.readValue();
    this.firmwareVersion = decodeCString(new Uint8Array(view.buffer));
  }

  private async readWiFiStatus() {
    const view = await this.wifiStatusCharacteristic!.readValue();
    this.wifiStatusChanged(view);
  }

  private async readWiFiDetails() {
    const view = await this.wifiDetailsCharacteristic!.readValue();
    this.wifiDetailsChanged(view);
  }

  private async readWiFiScanStatus() {
    const view = await this.wifiScanStatusCharacteristic!.readValue();
    this.wifiScanStatusChanged(view);
  }

  private async readWiFiScanResult() {
    const view = await this.wifiScanResultCharacteristic!.readValue();
    this.wifiScanResultChanged(view);
  }

  private async readAlexaIntegration() {
    const view = await this.alexaCharacteristic!.readValue();
    const alexaDetails = decodeAlexaIntegrationSettings(new Uint8Array(view.buffer));
    this.resetAlexaIntegrationForm(alexaDetails.integrationMode);
    this.alexaIntegrationForm.reset(alexaDetails, {emitEvent: true});
  }

  async readAlexaColor() {
    this.readingAlexaColor = true;
    const view = await this.alexaColorCharacteristic!.readValue();
    this.alexaColorChanged(view);
    this.readingAlexaColor = false;
  }

  private async sendWifiConfig(details: Uint8Array) {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.wifiStatusCharacteristic!.writeValue(details);
      this.snackBar.open('WiFi configuration sent', 'Close', {duration: 3000});
    } catch (e) {
      console.error('Failed to send WiFi configuration:', e);
      this.snackBar.open('Failed to send WiFi configuration', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  toggleDeviceEnabled(controlName: string) {
    const control = this.alexaIntegrationForm.get(controlName);
    if (!control) return;
    if (control.disabled) {
      control.enable();
    } else {
      control.setValue('');
      control.disable();
    }
  }

  resetAlexaIntegrationForm(mode: AlexaIntegrationMode) {
    this.alexaIntegrationForm.reset({integrationMode: mode});
    switch (mode) {
      case AlexaIntegrationMode.OFF:
        this.alexaIntegrationForm.controls.rDeviceName.disable();
        this.alexaIntegrationForm.controls.gDeviceName.disable();
        this.alexaIntegrationForm.controls.bDeviceName.disable();
        this.alexaIntegrationForm.controls.wDeviceName.disable();
        break;
      case AlexaIntegrationMode.RGBW_DEVICE:
        this.alexaIntegrationForm.controls.rDeviceName.enable();
        this.alexaIntegrationForm.controls.gDeviceName.disable();
        this.alexaIntegrationForm.controls.bDeviceName.disable();
        this.alexaIntegrationForm.controls.wDeviceName.disable();
        break;
      case AlexaIntegrationMode.RGB_DEVICE:
        this.alexaIntegrationForm.controls.rDeviceName.enable();
        this.alexaIntegrationForm.controls.gDeviceName.disable();
        this.alexaIntegrationForm.controls.bDeviceName.disable();
        this.alexaIntegrationForm.controls.wDeviceName.enable();
        break;
      case AlexaIntegrationMode.MULTI_DEVICE:
        this.alexaIntegrationForm.controls.rDeviceName.enable();
        this.alexaIntegrationForm.controls.gDeviceName.enable();
        this.alexaIntegrationForm.controls.bDeviceName.enable();
        this.alexaIntegrationForm.controls.wDeviceName.enable();
        break;
    }
  }
}
