import { ChangeDetectionStrategy, Component, input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Device } from './bluetooth.service';

@Component({
  selector: 'app-device-info',
  imports: [CommonModule],
  template: `
    @if (device(); as dev) {
      <div class="device-card">
        <div class="device-name">{{ dev.name }}</div>
        <div class="device-id">ID: {{ dev.id }}</div>
        <div class="rssi-container">
          <div class="rssi-value">{{ dev.rssi?.toFixed(0) ?? 'N/A' }} <span>dBm</span></div>
          <div class="rssi-label">Força do Sinal</div>
        </div>
        <div class="distance-container">
          <div class="distance-value">{{ dev.distance?.toFixed(1) ?? 'N/A' }} <span>m</span></div>
          <div class="distance-category">{{ dev.distanceCategory ?? 'Calculando...' }}</div>
        </div>
      </div>
    }
  `,
  styles: [`
    :host {
      display: block;
      width: 100%;
      max-width: 500px;
    }

    .device-card {
      background: var(--card-background);
      border-radius: 15px;
      padding: 25px;
      color: var(--text-color);
      display: flex;
      flex-direction: column;
      gap: 15px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
      border: 1px solid rgba(255, 255, 255, 0.1);
    }

    .device-name {
      font-size: 1.8em;
      font-weight: bold;
      text-align: center;
      color: var(--primary-color-start);
    }

    .device-id {
      font-size: 0.8em;
      text-align: center;
      color: var(--text-color-dark);
      margin-top: -10px;
      margin-bottom: 10px;
    }

    .rssi-container, .distance-container {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 15px;
      background: rgba(0,0,0,0.2);
      border-radius: 10px;
    }

    .rssi-value, .distance-value {
      font-size: 2em;
      font-weight: 300;
    }

    .rssi-value span, .distance-value span {
      font-size: 0.5em;
      color: var(--text-color-dark);
    }

    .rssi-label, .distance-category {
      font-size: 1em;
      text-align: right;
      color: var(--text-color-dark);
    }
  `],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class DeviceInfoComponent {
  device = input.required<Device>();
}
