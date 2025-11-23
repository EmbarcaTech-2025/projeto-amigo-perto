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

  // Signal de status para geolocalização
  public panicStatus = signal<{ message: string; type: 'info' | 'error' } | null>(null);

  /**
   * Aciona o botão de pânico para abrir o Google Maps com a localização atual.
   */
  onPanic(): void {
    this.panicStatus.set({ message: 'Obtendo sua localização para abrir o mapa...', type: 'info' });

    if (!navigator.geolocation) {
      this.panicStatus.set({ message: 'Geolocalização não é suportada por este navegador.', type: 'error' });
      return;
    }

    navigator.geolocation.getCurrentPosition(
      (position) => {
        const { latitude, longitude } = position.coords;
        const googleMapsUrl = `https://www.google.com/maps?q=${latitude},${longitude}`;

        // Abre o Google Maps em uma nova janela/aba.
        window.open(googleMapsUrl, '_blank');

        // Limpa a mensagem de status após um tempo para uma UI mais limpa
        setTimeout(() => this.panicStatus.set(null), 5000);
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
}
