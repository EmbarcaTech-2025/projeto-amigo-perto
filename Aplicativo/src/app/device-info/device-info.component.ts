import { ChangeDetectionStrategy, Component, input, output } from '@angular/core';

@Component({
  selector: 'app-device-info',
  templateUrl: './device-info.component.html',
  styleUrls: ['./device-info.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class DeviceInfoComponent {
  device = input.required<BluetoothDevice>();
  disconnect = output<void>();

  onDisconnect() {
    this.disconnect.emit();
  }
}
