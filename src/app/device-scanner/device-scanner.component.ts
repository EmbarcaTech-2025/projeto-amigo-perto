import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { BluetoothService } from '../bluetooth.service';

@Component({
  selector: 'app-device-scanner',
  imports: [CommonModule],
  template: `
    <div class="scan-controls">
      <button (click)="toggleScan()" [class.scanning]="isScanning()">
        @if(isScanning()) {
          <span><i class="scanner-dot"></i>Parar Busca</span>
        } @else {
          <span>Procurar Dispositivo</span>
        }
      </button>
    </div>
  `,
  styles: [`
    .scan-controls {
      margin-bottom: 20px;
    }

    button {
      background: linear-gradient(45deg, var(--primary-color), var(--secondary-color));
      color: white;
      border: none;
      padding: 18px 36px;
      font-size: 1.1rem;
      font-weight: 600;
      border-radius: 50px; /* Botão mais arredondado, estilo "pill" */
      cursor: pointer;
      transition: transform 0.2s ease-out, box-shadow 0.3s ease-out;
      box-shadow: 0 4px 15px var(--shadow-color);
      position: relative;
      overflow: hidden; /* Para o efeito de brilho */
    }

    button::before {
        content: '';
        position: absolute;
        top: 50%;
        left: 50%;
        width: 300%;
        height: 300%;
        background: rgba(255, 255, 255, 0.15);
        border-radius: 50%;
        transform: translate(-50%, -50%) scale(0);
        transition: transform 0.7s ease;
    }

    button:hover::before {
        transform: translate(-50%, -50%) scale(1);
    }

    button:hover {
      transform: translateY(-3px);
      box-shadow: 0 8px 25px rgba(93, 23, 230, 0.3);
    }

    button.scanning {
      background: linear-gradient(45deg, var(--danger-color-start), var(--danger-color-end));
    }

    button.scanning:hover {
        box-shadow: 0 8px 25px rgba(213, 51, 105, 0.4);
    }

    .scanner-dot {
        display: inline-block;
        width: 8px;
        height: 8px;
        background-color: white;
        border-radius: 50%;
        margin-right: 10px;
        animation: pulse 1.5s infinite;
    }

    @keyframes pulse {
        0% { box-shadow: 0 0 0 0 rgba(255,255,255, 0.7); }
        70% { box-shadow: 0 0 0 10px rgba(255,255,255, 0); }
        100% { box-shadow: 0 0 0 0 rgba(255,255,255, 0); }
    }

    span {
        display: flex;
        align-items: center;
        justify-content: center;
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
