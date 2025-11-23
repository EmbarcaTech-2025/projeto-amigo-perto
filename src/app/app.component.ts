import { ChangeDetectionStrategy, Component, inject, signal } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { BluetoothService } from './bluetooth.service';
import { DeviceInfoComponent } from './device-info/device-info.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  imports: [CommonModule, DeviceInfoComponent, NgOptimizedImage],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class AppComponent {
  public bluetoothService = inject(BluetoothService);

  // Signal de status para geolocalização (mais robusto)
  public panicStatus = signal<{ message: string; type: 'success' | 'error' } | null>(null);

  /**
   * Aciona o fluxo do botão de pânico com lógica refatorada.
   */
  onPanic(): void {
    this.panicStatus.set(null);

    if (!navigator.geolocation) {
      this.panicStatus.set({ message: 'Geolocalização não é suportada por este navegador.', type: 'error' });
      return;
    }

    navigator.geolocation.getCurrentPosition(
      (position) => {
        console.log('Localização obtida:', position.coords);
        this.simulateEmergencyDispatch(position.coords);
      },
      (error) => {
        let errorMessage = 'Ocorreu um erro ao obter a localização.';
        switch (error.code) {
          case error.PERMISSION_DENIED:
            errorMessage = 'Permissão para acessar a localização foi negada.';
            break;
          case error.POSITION_UNAVAILABLE:
            errorMessage = 'Informações de localização não estão disponíveis.';
            break;
          case error.TIMEOUT:
            errorMessage = 'A solicitação para obter a localização expirou.';
            break;
        }
        console.error(errorMessage, error);
        this.panicStatus.set({ message: errorMessage, type: 'error' });
      }
    );
  }

  /**
   * Simula o envio de um alerta de emergência e atualiza o status.
   */
  private simulateEmergencyDispatch(coords: GeolocationCoordinates): void {
    const message = `ALERTA DE PÂNICO: Localização de emergência enviada. Coordenadas: Latitude ${coords.latitude.toFixed(6)}, Longitude ${coords.longitude.toFixed(6)}`;
    setTimeout(() => {
      this.panicStatus.set({ message, type: 'success' });
    }, 1000); // Simula uma chamada de rede
  }
}
