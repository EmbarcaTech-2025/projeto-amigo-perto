import { ChangeDetectionStrategy, Component, computed, input } from '@angular/core';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-rssi-radar',
  imports: [CommonModule],
  templateUrl: './rssi-radar.html',
  styleUrls: ['./rssi-radar.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class RssiRadarComponent {
  // O valor do RSSI agora é uma entrada obrigatória.
  public rssi = input.required<number | null>();

  // A lógica de proximidade continua a mesma, mas agora reage à entrada.
  public proximity = computed(() => {
    const rssiValue = this.rssi();
    if (rssiValue === null) {
      return 'Escaneando'; // Estado inicial antes de receber o primeiro sinal
    }
    if (rssiValue > -50) {
      return 'Perto';
    } else if (rssiValue > -80) {
      return 'Média';
    } else {
      return 'Longe';
    }
  });
}
