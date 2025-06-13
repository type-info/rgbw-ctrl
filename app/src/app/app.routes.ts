import { Routes } from '@angular/router';

export const routes: Routes = [
  { path: '', redirectTo: 'rgbw-ctrl', pathMatch: 'full' },
  { path: 'rgbw-ctrl', title: 'rgbw-ctrl', loadComponent: () => import('./rgbw-ctrl/rgbw-ctrl.component').then(m => m.RgbwCtrlComponent) },
];
