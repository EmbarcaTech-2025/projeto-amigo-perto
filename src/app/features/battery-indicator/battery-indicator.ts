
import { ChangeDetectionStrategy, Component, computed, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { BluetoothService } from '../../bluetooth.service';

@Component({
  selector: 'app-battery-indicator',
  imports: [CommonModule],
  templateUrl: './battery-indicator.html',
  styleUrls: ['./battery-indicator.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class BatteryIndicatorComponent {
  private bluetoothService = inject(BluetoothService);

  // Acessa diretamente o sinal do serviço.
  public level = this.bluetoothService.batteryLevel;

  // Calcula a classe de cor com base no nível da bateria.
  public colorClass = computed(() => {
    const level = this.level();
    if (level === null) return '';
    if (level > 70) return 'high';
    if (level > 20) return 'medium';
    return 'low';
  });

  // Determina qual ícone de bateria mostrar.
  public icon = computed(() => {
    const level = this.level();
    if (level === null) return 'battery_unknown'; 
    if (level > 95) return 'battery_full';
    if (level > 85) return 'battery_6_bar';
    if (level > 70) return 'battery_5_bar';
    if (level > 55) return 'battery_4_bar';
    if (level > 40) return 'battery_3_bar';
    if (level > 25) return 'battery_2_bar';
    if (level > 10) return 'battery_1_bar';
    return 'battery_alert'; // Menos de 10%
  });
}
