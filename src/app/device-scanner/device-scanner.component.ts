import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { BluetoothService } from '../bluetooth.service';

@Component({
  selector: 'app-device-scanner',
  imports: [CommonModule],
  template: `
    <div class="scan-controls">
      <button (click)="toggleScan()" [class.scanning]="isScanning()">
        {{ isScanning() ? 'Parar Busca' : 'Procurar Dispositivo' }}
      </button>
    </div>
  `,
  styles: [`
    .scan-controls {
      margin-bottom: 20px;
    }
    button {
      background-color: #007bff;
      color: white;
      border: none;
      padding: 15px 30px;
      font-size: 16px;
      border-radius: 5px;
      cursor: pointer;
      transition: background-color 0.3s, box-shadow 0.3s;
    }
    button:hover {
      background-color: #0056b3;
      box-shadow: 0 4px 8px rgba(0,0,0,0.2);
    }
    button.scanning {
      background-color: #dc3545;
    }
    button.scanning:hover {
      background-color: #c82333;
    }
  `],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class DeviceScannerComponent {
  private bluetoothService = inject(BluetoothService);
  public isScanning = this.bluetoothService.scanning;

  toggleScan(): void {
    this.bluetoothService.startScan();
  }
}
