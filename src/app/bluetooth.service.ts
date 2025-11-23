import { Injectable, signal, inject, NgZone } from '@angular/core';

// --- Type Definitions for Web Bluetooth API ---
// Adicionadas para garantir a robustez da compilação
interface BluetoothDevice extends EventTarget {
  readonly id: string;
  readonly name?: string | undefined;
  readonly gatt?: BluetoothRemoteGATTServer | undefined;
  watchAdvertisements(options?: any): Promise<void>;
}

interface BluetoothRemoteGATTServer {
  readonly device: BluetoothDevice;
  readonly connected: boolean;
  connect(): Promise<BluetoothRemoteGATTServer>;
  disconnect(): void;
  getPrimaryService(service: string): Promise<BluetoothRemoteGATTService>;
}

interface BluetoothRemoteGATTService {
  readonly device: BluetoothDevice;
  readonly uuid: string;
  getCharacteristic(characteristic: string): Promise<BluetoothRemoteGATTCharacteristic>;
}

interface BluetoothRemoteGATTCharacteristic extends EventTarget {
  readonly service: BluetoothRemoteGATTService;
  readonly uuid: string;
  readonly value?: DataView | undefined;
  readValue(): Promise<DataView>;
  writeValue(value: BufferSource): Promise<void>;
  writeValueWithoutResponse(value: BufferSource): Promise<void>;
}

interface BluetoothAdvertisingEvent extends Event {
  readonly device: BluetoothDevice;
  readonly rssi?: number | undefined;
  readonly txPower?: number | undefined;
}


export interface Device {
  name: string;
  id: string;
  rssi?: number;
  distance?: number;
  distanceCategory?: string;
}

// --- Constantes de Rastreamento e Alerta ---
const IMMEDIATE_ALERT_SERVICE_UUID = '00001802-0000-1000-8000-00805f9b34fb';
const ALERT_LEVEL_CHARACTERISTIC_UUID = '00002a06-0000-1000-8000-00805f9b34fb';

// --- Constantes de Calibração de Distância (RSSI) ---
const MEASURED_POWER_AT_1M = -52; // Potência do sinal (em dBm) medida a 1 metro de distância.
const ENVIRONMENTAL_FACTOR = 4;   // Fator ambiental. Varia de 2 (espaço aberto) a 4 (ambientes internos com paredes).
const RSSI_OUT_OF_RANGE_THRESHOLD = -100; // Limite de RSSI para considerar "fora de alcance".


export type OperatingMode = 'idle' | 'radar' | 'alert';

@Injectable({
  providedIn: 'root',
})
export class BluetoothService {
  private zone = inject(NgZone);
  private bluetoothDevice: BluetoothDevice | null = null;
  private alertLevelCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private watchAdvertisementsController: AbortController | null = null;
  private audioContext: AudioContext | null = null;
  private alertIntervalId: any = null;

  // --- Sinais Públicos de Estado ---
  device = signal<Device | null>(null);
  operatingMode = signal<OperatingMode>('idle');
  error = signal<string | null>('Pronto para iniciar. Clique para procurar um dispositivo.');
  isOutOfRange = signal<boolean>(false);
  isLoading = signal<boolean>(false); // Signal para o estado de carregamento

  // --- Ações Públicas ---

  async startRadarMode(): Promise<void> {
    if (this.operatingMode() !== 'idle') return;

    if (!navigator.bluetooth) {
      this.error.set('Web Bluetooth não é suportado neste navegador.');
      return;
    }

    this.error.set('Procurando seu dispositivo... Por favor, selecione-o na janela.');

    try {
      this.bluetoothDevice = await navigator.bluetooth.requestDevice({
        acceptAllDevices: true,
        optionalServices: [IMMEDIATE_ALERT_SERVICE_UUID],
      });

      this.device.set({
        name: this.bluetoothDevice.name ?? 'Dispositivo Desconhecido',
        id: this.bluetoothDevice.id,
      });

      this.bluetoothDevice.addEventListener('gattserverdisconnected', this.onDisconnected);

      this.watchAdvertisementsController = new AbortController();

      await this.bluetoothDevice.watchAdvertisements({ signal: this.watchAdvertisementsController.signal });
      
      this.bluetoothDevice.addEventListener('advertisementreceived', this.advertisementListener, {
        once: false
      });

      this.zone.run(() => {
          this.operatingMode.set('radar');
          this.error.set('Modo Radar: Monitorando proximidade do dispositivo.');
      });

    } catch (error: any) {
      this.handleError(error);
    }
  }

  async switchToAlertMode(): Promise<void> {
    if (this.operatingMode() !== 'radar' || !this.bluetoothDevice) return;

    this.isLoading.set(true); // Inicia o carregamento
    this.stopWatchingAdvertisements();
    this.error.set('Mudando para Modo Alerta... Conectando...');

    try {
      const server = await this.bluetoothDevice.gatt!.connect();
      const service = await server.getPrimaryService(IMMEDIATE_ALERT_SERVICE_UUID);
      this.alertLevelCharacteristic = await service.getCharacteristic(ALERT_LEVEL_CHARACTERISTIC_UUID);

      this.zone.run(() => {
        this.operatingMode.set('alert');
        this.error.set('Modo Alerta: Pronto para enviar alertas ao iTag. O RSSI congela neste modo.');
      });
    } catch (error: any) {
      this.error.set(`Falha ao conectar: ${error.message}`);
      this.startRadarModeAfterFailure();
    } finally {
      this.isLoading.set(false); // Finaliza o carregamento (sucesso ou falha)
    }
  }

  disconnect(): void {
    this.stopWatchingAdvertisements();

    if (this.bluetoothDevice?.gatt?.connected) {
      this.bluetoothDevice.gatt.disconnect();
    } else {
      this.onDisconnected(); 
    }
  }

  async sendAlert(level: 0 | 1 | 2): Promise<void> {
    if (this.operatingMode() !== 'alert' || !this.alertLevelCharacteristic) {
      this.error.set('Não está em modo de alerta ou característica não está disponível.');
      return;
    }
    try {
      await this.alertLevelCharacteristic.writeValueWithoutResponse(Uint8Array.of(level));
    } catch (error: any) {
      this.error.set(`Erro ao enviar alerta: ${error.message}`);
    }
  }

  // --- Handlers e Métodos Privados ---

  private advertisementListener = (event: Event) => {
    const advEvent = event as BluetoothAdvertisingEvent;

    this.zone.run(() => {
      if (this.operatingMode() !== 'radar') return;

      const { rssi } = advEvent;
      if (rssi === undefined) return;

      const distance = this.calculateDistance(rssi);
      const distanceCategory = this.getDistanceCategory(distance);

      const wasOutOfRange = this.isOutOfRange();
      const isNowOutOfRange = rssi < RSSI_OUT_OF_RANGE_THRESHOLD;

      this.isOutOfRange.set(isNowOutOfRange);

      if (isNowOutOfRange && !wasOutOfRange) {
        this.playOutOfRangeCycle();
      } else if (!isNowOutOfRange && wasOutOfRange) {
        this.stopOutOfRangeAlertCycle();
      }

      this.device.update(current => ({
        ...current!,
        rssi: rssi,
        distance: distance,
        distanceCategory: distanceCategory,
      }));
    });
  };

  private onDisconnected = () => {
    this.zone.run(() => {
      this.stopWatchingAdvertisements();
      this.stopOutOfRangeAlertCycle();
      this.device.set(null);
      this.isOutOfRange.set(false);
      this.bluetoothDevice?.removeEventListener('gattserverdisconnected', this.onDisconnected);
      this.bluetoothDevice = null;
      this.alertLevelCharacteristic = null;
      this.operatingMode.set('idle');
      this.error.set('Dispositivo desconectado. Pronto para uma nova busca.');
    });
  }

  private stopWatchingAdvertisements() {
      if (this.watchAdvertisementsController) {
        this.watchAdvertisementsController.abort();
        this.watchAdvertisementsController = null;
      }
      if (this.bluetoothDevice) {
        this.bluetoothDevice.removeEventListener('advertisementreceived', this.advertisementListener);
      }
  }

  private async startRadarModeAfterFailure() {
    this.operatingMode.set('idle');
    await this.startRadarMode();
  }

  private handleError(error: any) {
    this.zone.run(() => {
      if (error.name !== 'AbortError' && error.name !== 'NotFoundError') {
        this.error.set(`Erro: ${error.message}`);
      }
      this.onDisconnected();
    });
  }

  /**
   * Calcula a distância aproximada em metros com base no RSSI.
   * Usa o modelo de path loss de log a distância.
   */
  private calculateDistance(rssi: number): number {
    const exponent = (MEASURED_POWER_AT_1M - rssi) / (10 * ENVIRONMENTAL_FACTOR);
    const distance = Math.pow(10, exponent);
    return parseFloat(distance.toFixed(2));
  }

  /**
   * Retorna uma categoria de distância com base na distância calculada em metros.
   */
  private getDistanceCategory(distance: number): string {
    if (distance <= 0.5) return "Muito Perto (Toque)"; 
    if (distance <= 2) return "Perto (Mesmo cômodo)";
    if (distance <= 10) return "Médio (Casa ou escritório)";
    return `Longe (Mais de 10 metros)`;
  }


  private beep() {
    if (!this.audioContext) {
        try {
            this.audioContext = new AudioContext();
        } catch (e) {
            this.error.set('Alerta sonoro não suportado neste navegador.');
            return;
        }
    }
    const oscillator = this.audioContext.createOscillator();
    const gainNode = this.audioContext.createGain();
    oscillator.connect(gainNode);
    gainNode.connect(this.audioContext.destination);

    oscillator.type = 'sine';
    oscillator.frequency.setValueAtTime(987.77, this.audioContext.currentTime); // Nota B5
    gainNode.gain.setValueAtTime(0.5, this.audioContext.currentTime);

    oscillator.start();
    oscillator.stop(this.audioContext.currentTime + 0.2); 
  }

  private playOutOfRangeCycle() {
    if (this.alertIntervalId) return; 

    let alertCount = 0;
    const maxAlerts = 5;

    this.beep();
    alertCount++;

    this.alertIntervalId = setInterval(() => {
      if (alertCount >= maxAlerts) {
        this.stopOutOfRangeAlertCycle();
        return;
      }
      this.beep();
      alertCount++;
    }, 1000); 
  }

  private stopOutOfRangeAlertCycle() {
    if (this.alertIntervalId) {
      clearInterval(this.alertIntervalId);
      this.alertIntervalId = null;
    }
  }
}
