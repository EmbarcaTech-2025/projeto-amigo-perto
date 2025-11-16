import { ChangeDetectionStrategy, Component, input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Device } from '../bluetooth.service';

@Component({
  selector: 'app-device-details',
  imports: [CommonModule],
  template: `
    @if (device(); as dev) {
      <div class="device-info-card">
        <h2>{{ dev.name }}</h2>
        <div class="device-stats">
            <div class="stat-item">
                <span class="stat-label">RSSI</span>
                <span class="stat-value">{{ dev.rssi }} dBm</span>
            </div>
            <div class="stat-item">
                <span class="stat-label">Distância</span>
                <span class="stat-value">{{ dev.distanceCategory }}</span>
            </div>
        </div>
      </div>
    }
  `,
  styles: [`
    .device-info-card {
      background-color: var(--card-background);
      border-radius: 16px;
      padding: 24px;
      margin-top: 30px;
      box-shadow: 0 10px 30px -15px var(--shadow-color);
      text-align: left;
      transition: transform 0.3s ease, box-shadow 0.3s ease;
    }

    .device-info-card:hover {
        transform: translateY(-5px);
        box-shadow: 0 12px 35px -10px rgba(0, 0, 0, 0.15);
    }

    h2 {
        margin: 0 0 20px 0;
        text-align: center;
        color: var(--primary-color);
        font-weight: 700;
    }

    .device-stats {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 20px;
    }

    .stat-item {
        display: flex;
        flex-direction: column;
        align-items: center;
        background-color: #f9f9f9;
        padding: 15px;
        border-radius: 12px;
    }

    .stat-label {
        font-size: 0.9rem;
        color: #888;
        margin-bottom: 8px;
        font-weight: 600;
    }

    .stat-value {
        font-size: 1.2rem;
        font-weight: 700;
        color: var(--secondary-color); /* Cor padronizada */
    }
  `],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class DeviceDetailsComponent {
  device = input<Device | null>();
}
