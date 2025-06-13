import {Component} from '@angular/core';
import {MatButtonModule} from '@angular/material/button';
import {MatDialogModule} from '@angular/material/dialog';
import {MatInputModule} from '@angular/material/input';
import {FormControl, FormGroup, ReactiveFormsModule, Validators} from '@angular/forms';
import {MatFormFieldModule} from '@angular/material/form-field';
import {MatSelectModule} from '@angular/material/select';
import {WiFiEncryptionType} from '../../wifi.model';

@Component({
  selector: 'app-custom-wi-fi-connect-dialog',
  imports: [
    MatButtonModule,
    MatDialogModule,
    MatFormFieldModule,
    MatSelectModule,
    MatInputModule,
    ReactiveFormsModule,
  ],
  templateUrl: './custom-wi-fi-connect-dialog.component.html',
  styleUrl: './custom-wi-fi-connect-dialog.component.scss'
})
export class CustomWiFiConnectDialogComponent {

  readonly WiFiEncryptionType = WiFiEncryptionType;

  form = new FormGroup({
    encryptionType: new FormControl<WiFiEncryptionType>(WiFiEncryptionType.WIFI_AUTH_OPEN, {
      nonNullable: true,
      validators: [Validators.required],
    })
  });
}
