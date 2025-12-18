import { ChangeDetectionStrategy, Component, inject, computed } from '@angular/core';

import { BluetoothService, Proximity } from './bluetooth.service';

@Component({
  selector: 'app-rssi-radar',
  imports: [],
  templateUrl: './rssi-radar.component.html',
  styleUrls: ['./rssi-radar.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class RssiRadarComponent {
  public bluetoothService = inject(BluetoothService);

  readonly shouldShake = computed(() => this.bluetoothService.proximity() === 'longe');

  getProximityText(proximity: Proximity): string {
    switch (proximity) {
      case 'perto':
        return 'Muito Perto';
      case 'medio':
        return 'Distância Média';
      case 'longe':
        return 'Sinal Fraco - Risco de Perda';
      default:
        return 'Procurando sinal...';
    }
  }

  connect() {
    this.bluetoothService.connect();
  }
}
