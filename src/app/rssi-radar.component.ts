import { CommonModule } from '@angular/common';
import { ChangeDetectionStrategy, Component, computed, input } from '@angular/core';

type Proximity = 'unknown' | 'longe' | 'medio' | 'perto';

@Component({
  selector: 'app-rssi-radar',
  templateUrl: './rssi-radar.component.html',
  styleUrls: ['./rssi-radar.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [CommonModule]
})
export class RssiRadarComponent {
  public rssi = input<number | null>(null);

  public proximity = computed<Proximity>(() => {
    const rssiVal = this.rssi();
    if (rssiVal === null) return 'unknown';
    if (rssiVal < -80) return 'longe';
    if (rssiVal < -60) return 'medio';
    return 'perto';
  });

  public proximityText = computed(() => {
    switch (this.proximity()) {
      case 'perto':
        return 'Muito Perto';
      case 'medio':
        return 'Distância Média';
      case 'longe':
        return 'Sinal Fraco - Risco de Perda';
      default:
        return 'Procurando sinal...';
    }
  });

  public shouldShake = computed(() => this.proximity() === 'longe');
}
