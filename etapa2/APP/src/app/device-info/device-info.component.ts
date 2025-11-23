import { ChangeDetectionStrategy, Component, input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Device } from '../bluetooth.service';

@Component({
  selector: 'app-device-info',
  imports: [CommonModule],
  template: `
    @if (device(); as dev) {
      <div class="device-card">
        <div class="device-name">{{ dev.name || 'Dispositivo Desconhecido' }}</div>
        
        @if (dev.rssi !== undefined && dev.distance !== undefined) {
          <div class="device-stats">
            <div class="stat-item">
              <span class="stat-label">RSSI</span>
              <span class="stat-value">{{ dev.rssi }} dBm</span>
            </div>
            <div class="stat-item">
              <span class="stat-label">Distância</span>
              <span class="stat-value">~{{ dev.distance }} m</span>
            </div>
          </div>
          @if(dev.distanceCategory) {
            <div class="distance-category">{{ dev.distanceCategory }}</div>
          }
        } @else {
          <div class="device-status">Conectado</div>
        }
      </div>
    }
  `,
  styles: `
    :host {
      display: block;
      width: 100%;
    }
    .device-card {
      background-color: var(--card-background);
      color: var(--text-color);
      padding: 20px;
      border-radius: 12px;
      text-align: center;
      margin-bottom: 20px;
      border: 1px solid rgba(255, 255, 255, 0.1);
      font-family: 'Inter', sans-serif;
    }
    .device-name {
      font-size: 1.4em;
      font-weight: bold;
      margin-bottom: 16px;
    }
    .device-stats {
      display: flex;
      justify-content: space-around;
      margin-bottom: 16px;
      gap: 10px;
    }
    .stat-item {
      display: flex;
      flex-direction: column;
      align-items: center;
      background: rgba(0,0,0,0.2);
      padding: 10px;
      border-radius: 8px;
      flex: 1;
    }
    .stat-label {
      font-size: 0.8em;
      color: var(--text-secondary-color);
      margin-bottom: 4px;
      text-transform: uppercase;
    }
    .stat-value {
      font-size: 1.2em;
      font-weight: 500;
      color: var(--accent-color);
    }
    .distance-category {
        margin-top: 10px;
        font-size: 1em;
        color: var(--text-color);
        font-style: italic;
    }
    .device-status {
      font-size: 1em;
      color: var(--success-color);
    }
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class DeviceInfoComponent {
  public device = input.required<Device>();
}
