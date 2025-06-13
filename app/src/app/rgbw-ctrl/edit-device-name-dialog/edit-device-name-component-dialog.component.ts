import {Component, Inject} from '@angular/core';
import {MAT_DIALOG_DATA, MatDialogModule} from '@angular/material/dialog';
import {FormControl, ReactiveFormsModule, Validators} from '@angular/forms';
import {MatFormFieldModule} from '@angular/material/form-field';
import {MatInputModule} from '@angular/material/input';
import {MatButtonModule} from '@angular/material/button';
import {CommonModule} from '@angular/common';
import {MatDialogRef} from '@angular/material/dialog';

@Component({
  selector: 'app-edit-device-name-dialog',
  imports: [
    CommonModule,
    MatFormFieldModule,
    MatInputModule,
    MatDialogModule,
    MatButtonModule,
    ReactiveFormsModule
  ],
  templateUrl: './edit-device-name-component-dialog.component.html',
  styleUrl: './edit-device-name-component-dialog.component.scss'
})
export class EditDeviceNameComponentDialog {

  readonly DEVICE_NAME_MAX_LENGTH = 28;
  readonly DEVICE_NAME_PREFIX = 'rgbw-ctrl-';

  deviceNameControl = new FormControl('', {
    nonNullable: true,
    validators: [
      Validators.required,
      Validators.minLength(1),
      Validators.maxLength(this.DEVICE_NAME_MAX_LENGTH),
      Validators.pattern(/^[a-zA-Z0-9-]+(?<!-)$/)
    ]
  });

  get value() {
    return this.deviceNameControl.value;
  }

  constructor(
    @Inject(MAT_DIALOG_DATA) deviceName: string,
    private matDialogRef: MatDialogRef<EditDeviceNameComponentDialog>
  ) {
    this.deviceNameControl.reset(deviceName.substring(this.DEVICE_NAME_PREFIX.length));
    this.deviceNameControl.markAsTouched();
  }

  submit() {
    if (this.deviceNameControl.invalid) return;
    this.matDialogRef.close(this.DEVICE_NAME_PREFIX + this.deviceNameControl.value);
  }
}
