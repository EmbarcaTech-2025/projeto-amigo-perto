import { bootstrapApplication } from '@angular/platform-browser';
import { AppComponent } from './app/app.component';
import { isDevMode } from '@angular/core';
import { enableBluetoothSimulation } from './app/bluetooth-simulation';

if (isDevMode()) {
  enableBluetoothSimulation();
}

bootstrapApplication(AppComponent).catch((err) => console.error(err));
