import { Routes } from '@angular/router';

export const routes: Routes = [
  { path: '', title: 'rgbw-ctrl', loadComponent: () => import('./rgbw-ctrl/rgbw-ctrl.component').then(m => m.RgbwCtrlComponent) },
];
