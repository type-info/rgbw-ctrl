import {Component, Inject} from '@angular/core';
import {MAT_DIALOG_DATA, MatDialogModule, MatDialogRef} from '@angular/material/dialog';
import {FormControl, FormGroup, ReactiveFormsModule, Validators} from '@angular/forms';
import {WiFiEncryptionType, WiFiNetwork} from '../../wifi.model';
import {MatFormFieldModule} from '@angular/material/form-field';
import {MatInputModule} from '@angular/material/input';
import {CommonModule} from '@angular/common';
import {MatButtonModule} from '@angular/material/button';

@Component({
  selector: 'app-simple-wi-fi-connect-dialog',
  imports: [
    CommonModule,
    MatDialogModule,
    MatFormFieldModule,
    MatButtonModule,
    MatInputModule,
    ReactiveFormsModule
  ],
  templateUrl: './simple-wi-fi-connect-dialog.component.html',
  styleUrl: './simple-wi-fi-connect-dialog.component.scss'
})
export class SimpleWiFiConnectDialogComponent {

  readonly MIN_WPA_PASSWORD_LENGTH = 8;
  readonly MAX_WPA_PASSWORD_LENGTH = 63;
  readonly MIN_WEP_ASCII_PASSWORD_LENGTH = 5;
  readonly MAX_WEP_HEX_PASSWORD_LENGTH = 26;
  readonly MIN_WEP_HEX_PASSWORD_LENGTH = 10;
  readonly MAX_WEP_ASCII_PASSWORD_LENGTH = 13;
  readonly WEP_HEX_PATTERN = /^[A-Fa-f0-9]+$/;

  readonly WifiAuthMode = WiFiEncryptionType;

  ssidReadonly = true;

  form = new FormGroup({
    encryptionType: new FormControl(WiFiEncryptionType.WIFI_AUTH_OPEN, {
      nonNullable: true,
      validators: [Validators.required]
    }),
    ssid: new FormControl('', {
      nonNullable: true,
      validators: [Validators.required]
    }),
    credentials: new FormGroup({
      password: new FormControl('', {
        nonNullable: true,
        validators: [] // Set in constructor
      })
    })
  });

  get password(): string {
    return this.form.controls.credentials.controls.password.value ?? '';
  }

  get minPasswordLength(): number | null {
    const type = this.form.controls.encryptionType.value;
    if (this.isWpaType(type)) {
      return this.MIN_WPA_PASSWORD_LENGTH;
    }
    if (type === WiFiEncryptionType.WIFI_AUTH_WEP) {
      const pwd = this.form.controls.credentials.controls.password.value ?? '';
      const isHex = this.WEP_HEX_PATTERN.test(pwd);
      return isHex ? this.MIN_WEP_HEX_PASSWORD_LENGTH : this.MIN_WEP_ASCII_PASSWORD_LENGTH;
    }
    return null;
  }

  get maxPasswordLength(): number | null {
    const type = this.form.controls.encryptionType.value;
    if (this.isWpaType(type)) {
      return this.MAX_WPA_PASSWORD_LENGTH;
    }
    if (type === WiFiEncryptionType.WIFI_AUTH_WEP) {
      const pwd = this.form.controls.credentials.controls.password.value ?? '';
      const isHex = this.WEP_HEX_PATTERN.test(pwd);
      return isHex ? this.MAX_WEP_HEX_PASSWORD_LENGTH : this.MAX_WEP_ASCII_PASSWORD_LENGTH;
    }
    return null;
  }

  constructor(
    @Inject(MAT_DIALOG_DATA) network: WiFiNetwork,
    private matDialogRef: MatDialogRef<SimpleWiFiConnectDialogComponent>
    ) {
    this.form.reset(network);

    if (!('ssid' in network)) {
      this.ssidReadonly = false;
    }

    const type = network.encryptionType;
    if (type === WiFiEncryptionType.WIFI_AUTH_OPEN) {
      this.form.controls.credentials.controls.password.clearValidators();
      this.form.controls.credentials.controls.password.setValidators([]);
    } else if (this.isWpaType(type)) {
      this.form.controls.credentials.controls.password.setValidators([
        Validators.required,
        Validators.minLength(this.MIN_WPA_PASSWORD_LENGTH),
        Validators.maxLength(this.MAX_WPA_PASSWORD_LENGTH),
        // pattern: all visible ASCII chars except spaces (optional, remove if you don't want to restrict)
        Validators.pattern(/^[\x21-\x7E]+$/)
      ]);
    } else if (type === WiFiEncryptionType.WIFI_AUTH_WEP) {
      this.form.controls.credentials.controls.password.setValidators([
        Validators.required,
        Validators.pattern(/^([A-Fa-f0-9]{10}|[A-Fa-f0-9]{26}|.{5}|.{13})$/)
      ]);
    } else {
      this.form.controls.credentials.controls.password.setValidators([Validators.required]);
    }
    this.form.controls.credentials.controls.password.updateValueAndValidity();
  }

  isWpaType(type: WiFiEncryptionType) {
    return [
      WiFiEncryptionType.WIFI_AUTH_WPA_PSK,
      WiFiEncryptionType.WIFI_AUTH_WPA2_PSK,
      WiFiEncryptionType.WIFI_AUTH_WPA_WPA2_PSK,
      WiFiEncryptionType.WIFI_AUTH_WPA3_PSK
    ].includes(type);
  }


  submit() {
    if (this.form.invalid) return;
    this.matDialogRef.close(this.form.value);
  }
}
