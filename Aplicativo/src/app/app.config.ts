import { ApplicationConfig } from '@angular/core';
import { BluetoothService } from './bluetooth.service';

export const appConfig: ApplicationConfig = {
  providers: [BluetoothService]
};
