import {Component, Inject} from '@angular/core';
import {MatButton, MatButtonModule} from "@angular/material/button";
import {
  MAT_DIALOG_DATA,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogModule,
  MatDialogTitle
} from "@angular/material/dialog";
import {MatInput, MatPrefix} from "@angular/material/input";
import {NgIf} from "@angular/common";
import {ReactiveFormsModule} from "@angular/forms";

@Component({
  selector: 'app-yes-no-dialog',
  imports: [
    MatDialogModule,
    MatButtonModule,
    ReactiveFormsModule
  ],
  templateUrl: './confirm-alexa-restart.component.html',
  styleUrl: './confirm-alexa-restart.component.scss'
})
export class ConfirmAlexaRestart {

  constructor() {
  }

}
