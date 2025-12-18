import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { BluetoothService } from './bluetooth.service';

@Component({
  selector: 'app-device-info',
  imports: [CommonModule],
  templateUrl: './device-info.component.html',
  styleUrls: ['./device-info.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class DeviceInfoComponent {
  bluetoothService = inject(BluetoothService);

  // Signals for local UI state
  isBuzzerOn = false;
  isLedOn = false;

  toggleBuzzer() {
    this.isBuzzerOn = !this.isBuzzerOn;
    this.bluetoothService.toggleBuzzer(this.isBuzzerOn);
  }

  toggleLed() {
    this.isLedOn = !this.isLedOn;
    this.bluetoothService.toggleLed(this.isLedOn);
  }

  reboot() {
    this.bluetoothService.rebootDevice();
  }

  requestBattery() {
    this.bluetoothService.requestBatteryLevelUpdate();
  }
}
