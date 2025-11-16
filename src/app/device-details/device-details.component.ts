import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
// Importa NgOptimizedImage
import { CommonModule, NgOptimizedImage } from '@angular/common'; 
// Importa o serviço e a interface Device para garantir a sincronia
import { BluetoothService, Device } from '../bluetooth.service';

@Component({
  selector: 'app-device-details',
  // Adiciona NgOptimizedImage aos imports do componente
  imports: [CommonModule, NgOptimizedImage],
  template: `
    <div class="device-details-container">
      @if(device(); as dev) {
        <div class="card">
          <!-- A condição @if usa a propriedade photoUrl, que agora existe -->
          @if(dev.photoUrl) {
            <!-- O [ngSrc] usa a mesma propriedade. Dimensões são obrigatórias -->
            <img [ngSrc]="dev.photoUrl" alt="" class="device-photo" width="250" height="250">
          }
          <div class="card-header">
            <h2>Detalhes do Dispositivo</h2>
          </div>
          <div class="card-body">
            <p><strong>Nome:</strong> {{ dev.name }}</p>
            @if(dev.rssi) {
              <p class="rssi"><strong>Força do Sinal (RSSI):</strong> {{ dev.rssi }} dBm</p>
            }
          </div>
        </div>
      } @else {
        <p class="no-device">Nenhum dispositivo selecionado.</p>
      }
    </div>
  `,
  styles: `
    .device-details-container {
      padding: 1rem;
    }
    .card {
      background-color: #fff;
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
      overflow: hidden;
      text-align: center;
    }
    .device-photo {
      width: 100%;
      max-height: 250px;
      height: auto; /* Comentário CSS corrigido */
      object-fit: cover;
    }
    .card-header {
      background-color: #007bff;
      color: white;
      padding: 1rem;
    }
    .card-body {
      padding: 1rem;
    }
    .rssi {
      font-weight: bold;
      color: #28a745;
      font-size: 1.1em;
    }
    .no-device {
      text-align: center;
      color: #6c757d;
      font-style: italic;
    }
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class DeviceDetailsComponent {
  private bluetoothService = inject(BluetoothService);
  public device = this.bluetoothService.selectedDevice;
}
