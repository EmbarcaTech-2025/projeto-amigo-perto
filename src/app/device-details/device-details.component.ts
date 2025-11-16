import { ChangeDetectionStrategy, Component, input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Device } from '../bluetooth.service';

@Component({
  selector: 'app-device-details',
  imports: [CommonModule],
  template: `
    @if (device(); as dev) {
      <div class="device-info">
        <h2>{{ dev.name }}</h2>
        <div class="device-stats">
            <p><strong>RSSI:</strong> {{ dev.rssi }} dBm</p>
            <p><strong>Distância Estimada:</strong> {{ dev.distanceCategory }}</p>
        </div>
      </div>
    }
  `,
  styles: [`
    .device-info {
      border: 1px solid #ddd;
      border-radius: 8px;
      padding: 20px;
      margin-top: 20px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .device-stats p {
      margin: 5px 0;
      font-size: 1.1em;
    }
  `],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class DeviceDetailsComponent {
  device = input<Device | null>();
}
