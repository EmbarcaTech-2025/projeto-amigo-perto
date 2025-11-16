import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { BluetoothService } from '../bluetooth.service';

@Component({
  selector: 'app-device-scanner',
  template: `
    <div class="scanner-container">
      <button (click)="scanForDevices()" [disabled]="isScanning()" class="scan-button">
        @if (isScanning()) {
          <span class="spinner"></span>
          Procurando...
        } @else {
          Encontrar Dispositivo
        }
      </button>
      @if (error()) {
        <p class="error-message">{{ error() }}</p>
      }
    </div>
  `,
  styles: `
    .scanner-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 1rem;
    }
    .scan-button {
      background-color: #007bff;
      color: white;
      border: none;
      border-radius: 5px;
      padding: 10px 20px;
      font-size: 1rem;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: background-color 0.3s ease;
    }
    .scan-button:disabled {
      background-color: #a0a0a0;
      cursor: not-allowed;
    }
    .spinner {
      border: 4px solid rgba(255, 255, 255, 0.3);
      border-radius: 50%;
      border-top-color: #fff;
      width: 16px;
      height: 16px;
      animation: spin 1s linear infinite;
      margin-right: 10px;
    }
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
    .error-message {
      color: #dc3545;
      margin-top: 1rem;
    }
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [CommonModule]
})
export class DeviceScannerComponent {
  private bluetoothService = inject(BluetoothService);

  public isScanning = this.bluetoothService.scanning;
  public error = this.bluetoothService.error;

  scanForDevices(): void {
    this.bluetoothService.scan();
  }
}
