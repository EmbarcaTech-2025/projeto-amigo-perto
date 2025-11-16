import { Injectable, signal, inject, NgZone } from '@angular/core';

// A interface Device DEVE ter a propriedade photoUrl
export interface Device {
  name: string;
  id: string;
  rssi?: number;
  photoUrl?: string;
  distance?: number;
  distanceCategory?: string;
}

// Parâmetros de Alerta e Distância
const RSSI_THRESHOLD = -80; // dBm. Limite de força do sinal para o alerta.
const TX_POWER = -59; // Potência de transmissão de referência a 1 metro.

@Injectable({
  providedIn: 'root',
})
export class BluetoothService {
  private zone = inject(NgZone);
  private audioContext: AudioContext | null = null;
  private abortController: AbortController | null = null;

  // --- Sinais Públicos ---
  device = signal<Device | null>(null);
  scanning = signal<boolean>(false);
  isOutOfRange = signal<boolean>(false);
  error = signal<string | null>(null);

  constructor() {
    // Inicializa o AudioContext de forma segura no navegador
    try {
      this.audioContext = new (window.AudioContext || (window as any).webkitAudioContext)();
    } catch (e) {
      console.warn('AudioContext não é suportado. Alertas sonoros desativados.');
      this.audioContext = null;
    }
  }

  // --- Ações Públicas ---
  async startScan(): Promise<void> {
    if (this.scanning()) {
      this.stopScan();
      return;
    }

    if (!navigator.bluetooth) {
      this.error.set('Erro: A API de Web Bluetooth não é suportada neste navegador.');
      return;
    }
    
    this.scanning.set(true);
    this.error.set('Procurando seu dispositivo... Por favor, selecione-o na janela.');
    this.abortController = new AbortController();
    let deviceSelected = false;

    try {
      const bluetoothDevice = await navigator.bluetooth.requestDevice({ acceptAllDevices: true });
      
      deviceSelected = true; // Device was selected by the user
      this.logStatus(`Monitorando ${bluetoothDevice.name || 'dispositivo'}...`);
      
      this.abortController.signal.addEventListener('abort', () => {
        this.zone.run(() => {
            // Check if the device object and its methods exist before using them
            if (bluetoothDevice && typeof bluetoothDevice.removeEventListener === 'function') {
                bluetoothDevice.removeEventListener('advertisementreceived', this.advertisementListener);
            }
            this.resetState();
        });
      });

      bluetoothDevice.addEventListener('advertisementreceived', this.advertisementListener);
      await bluetoothDevice.watchAdvertisements({ signal: this.abortController.signal });

    } catch (error: any) {
        this.zone.run(() => {
            // We only want to show a real error message, not cancellation messages.
            if (error.name !== 'AbortError' && error.name !== 'NotFoundError') {
                this.error.set(`Erro: ${error.message}`);
            }
        });
    } finally {
        // This block will run if `requestDevice` fails or when `watchAdvertisements` is aborted.
        // We only need to manually reset the state if the user never selected a device in the first place.
        // If a device was selected, the 'abort' event listener will handle the reset.
        if (!deviceSelected) {
            this.zone.run(() => {
                this.resetState();
            });
        }
    }
  }

  stopScan(): void {
    if (this.abortController) {
      this.abortController.abort();
      this.abortController = null;
    }
  }

  // --- Lógica de Alerta Sonoro ---
  beep(frequency = 800, duration = 150) {
    if (!this.audioContext) return;
    const oscillator = this.audioContext.createOscillator();
    const gainNode = this.audioContext.createGain();
    oscillator.connect(gainNode);
    gainNode.connect(this.audioContext.destination);
    oscillator.type = 'sine';
    oscillator.frequency.value = frequency;
    gainNode.gain.setValueAtTime(0, this.audioContext.currentTime);
    gainNode.gain.linearRampToValueAtTime(1, this.audioContext.currentTime + 0.02);
    oscillator.start(this.audioContext.currentTime);
    gainNode.gain.exponentialRampToValueAtTime(0.00001, this.audioContext.currentTime + duration / 1000);
    oscillator.stop(this.audioContext.currentTime + duration / 1000);
  }

  // --- Lógica Interna ---
  private advertisementListener = (event: BluetoothAdvertisingEvent) => {
    this.zone.run(() => {
        const { device, rssi } = event;
        if (rssi === undefined) return;

        const distance = this.calculateDistance(rssi);
        const distanceCategory = this.getDistanceCategory(distance);

        const updatedDevice: Device = {
            name: device.name ?? 'Dispositivo Desconhecido',
            id: device.id,
            rssi: rssi,
            distance: distance,
            distanceCategory: distanceCategory,
        };

        this.device.set(updatedDevice);
        this.logStatus('Dispositivo ao alcance.'); // Update status when receiving data

        // Lógica de Alerta
        if (rssi < RSSI_THRESHOLD) {
            if (!this.isOutOfRange()) {
                this.isOutOfRange.set(true);
                this.logStatus('ALERTA: Dispositivo muito distante!');
            }
        } else {
            this.isOutOfRange.set(false);
        }
    });
  };

  private calculateDistance(rssi: number): number {
    const ratio = rssi * 1.0 / TX_POWER;
    if (ratio < 1.0) {
      return parseFloat(Math.pow(ratio, 10).toFixed(2));
    } else {
      return parseFloat((0.89976 * Math.pow(ratio, 7.7095) + 0.111).toFixed(2));
    }
  }

  private getDistanceCategory(distance: number): string {
    if (distance <= 3) {
      return "~0 a 3 metros (Perto)";
    } else if (distance <= 10) {
      return "~3 a 10 metros (Médio)";
    } else {
      return `Mais de 10 metros (Longe)`;
    }
  }

  private resetState() {
    this.device.set(null);
    this.scanning.set(false);
    this.isOutOfRange.set(false);
    this.error.set('Busca interrompida. Clique para procurar novamente.');
  }

  private logStatus(message: string) {
      this.error.set(message); // Reutilizando o sinal de erro para status
  }
}
