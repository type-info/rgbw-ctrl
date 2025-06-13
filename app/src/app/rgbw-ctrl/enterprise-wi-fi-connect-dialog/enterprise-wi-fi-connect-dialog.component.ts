import {Component, Inject} from '@angular/core';
import {MAT_DIALOG_DATA, MatDialogModule, MatDialogRef} from '@angular/material/dialog';
import {FormControl, FormGroup, ReactiveFormsModule, Validators} from '@angular/forms';
import {WiFiNetwork, WiFiPhaseTwoType} from '../../wifi.model';
import {CommonModule} from '@angular/common';
import {MatFormFieldModule} from '@angular/material/form-field';
import {MatButtonModule} from '@angular/material/button';
import {MatInputModule} from '@angular/material/input';
import {MatSelectModule} from '@angular/material/select';

@Component({
  selector: 'app-enterprise-wi-fi-connect-dialog',
  templateUrl: './enterprise-wi-fi-connect-dialog.component.html',
  styleUrl: './enterprise-wi-fi-connect-dialog.component.scss',
  standalone: true,
  imports: [
    CommonModule,
    MatDialogModule,
    MatFormFieldModule,
    MatButtonModule,
    MatInputModule,
    MatSelectModule,
    ReactiveFormsModule
  ],
})
export class EnterpriseWiFiConnectDialogComponent {

  WiFiPhaseTwoType = WiFiPhaseTwoType;

  ssidReadonly = false;

  readonly phase2Options = [
    {value: WiFiPhaseTwoType.ESP_EAP_TTLS_PHASE2_EAP, label: 'EAP'},
    {value: WiFiPhaseTwoType.ESP_EAP_TTLS_PHASE2_MSCHAPV2, label: 'MSCHAPV2'},
    {value: WiFiPhaseTwoType.ESP_EAP_TTLS_PHASE2_MSCHAP, label: 'MSCHAP'},
    {value: WiFiPhaseTwoType.ESP_EAP_TTLS_PHASE2_PAP, label: 'PAP'},
    {value: WiFiPhaseTwoType.ESP_EAP_TTLS_PHASE2_CHAP, label: 'CHAP'}
  ] as const;

  form = new FormGroup({
    encryptionType: new FormControl(0, {nonNullable: true, validators: [Validators.required]}),
    ssid: new FormControl('', {nonNullable: true, validators: [Validators.required]}),
    credentials: new FormGroup({
      identity: new FormControl('', {nonNullable: true, validators: [Validators.required]}),
      username: new FormControl('', {nonNullable: true, validators: [Validators.required]}),
      password: new FormControl('', {nonNullable: true, validators: [Validators.required, Validators.maxLength(64)]}),
      phase2Type: new FormControl(0, {nonNullable: true, validators: [Validators.required]}),
    })
  });

  get password() {
    return this.form.controls.credentials.controls.password.value ?? '';
  }

  constructor(
    @Inject(MAT_DIALOG_DATA) network: WiFiNetwork,
    private matDialogRef: MatDialogRef<EnterpriseWiFiConnectDialogComponent>
    ) {
    this.form.reset(network);
    if (!('ssid' in network)) {
      this.ssidReadonly = false;
    }
  }

  submit() {
    if (this.form.invalid) return;
    this.matDialogRef.close(this.form.value);
  }

}
