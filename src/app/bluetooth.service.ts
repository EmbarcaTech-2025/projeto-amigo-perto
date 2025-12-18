
import { Injectable, signal, inject, NgZone, computed, WritableSignal } from '@angular/core';

// --- Type Definitions for Web Bluetooth API ---
interface WatchAdvertisementsOptions {
  signal?: AbortSignal;
}

interface BluetoothDevice extends EventTarget {
  readonly id: string;
  readonly name?: string | undefined;
  readonly gatt?: BluetoothRemoteGATTServer | undefined;
  watchAdvertisements(options?: WatchAdvertisementsOptions): Promise<void>;
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
  startNotifications(): Promise<BluetoothRemoteGATTCharacteristic>;
  stopNotifications(): Promise<BluetoothRemoteGATTCharacteristic>;
}

// --- UUID Definitions ---
const NUS_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHARACTERISTIC_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_TX_CHARACTERISTIC_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

const BATTERY_SERVICE_UUID = '0000180f-0000-1000-8000-00805f9b34fb';
const BATTERY_LEVEL_CHARACTERISTIC_UUID = '00002a19-0000-1000-8000-00805f9b34fb';

export interface Device {
  name: string;
  id: string;
}

export type ConnectionStatus = 'disconnected' | 'searching' | 'watching' | 'connecting' | 'connected';
export type Proximity = 'unknown' | 'longe' | 'medio' | 'perto';

@Injectable({
  providedIn: 'root',
})
export class BluetoothService {
  private zone = inject(NgZone);
  private textEncoder = new TextEncoder();
  private textDecoder = new TextDecoder();
  private audioContext: AudioContext | null = null;
  private abortController: AbortController | null = null;

  // --- Bluetooth State ---
  private bluetoothDevice: BluetoothDevice | null = null;
  private nusRxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private nusTxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private batteryLevelCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  // --- Public State Signals ---
  connectionStatus: WritableSignal<ConnectionStatus> = signal('disconnected');
  error: WritableSignal<string | null> = signal('Pronto para iniciar. Clique em "Buscar" para procurar um Amigo.');
  isLoading: WritableSignal<boolean> = signal(false);
  
  // Radar/RSSI State
  foundDevice = signal<Device | null>(null);
  rssi = signal<number | null>(null);
  isWatching = signal(false);

  // Connected Device State
  connectedDevice = signal<Device | null>(null);
  lastResponse = signal<string | null>(null);
  batteryLevel = signal<number | null>(null);

  // Computed Proximity
  proximity = computed(() => {
    const rssiVal = this.rssi();
    if (rssiVal === null) return 'unknown';
    if (rssiVal < -75) return 'longe';
    if (rssiVal < -60) return 'medio';
    return 'perto';
  });

  constructor() {
    try {
      this.audioContext = new ((window as any).AudioContext || (window as any).webkitAudioContext)();
    } catch (e) {
      console.warn('Web Audio API is not supported in this browser.');
    }
  }

  async startScan(): Promise<void> {
    if (!navigator.bluetooth) {
      this.error.set('Web Bluetooth não é suportado neste navegador.');
      return;
    }

    this.zone.run(() => {
      this.isLoading.set(true);
      this.connectionStatus.set('searching');
      this.error.set('Procurando Amigo... Por favor, selecione-o na janela.');
    });

    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [NUS_SERVICE_UUID] }],
        optionalServices: [BATTERY_SERVICE_UUID]
      });
      this.bluetoothDevice = device as unknown as BluetoothDevice;
      
      if (!this.bluetoothDevice) return;

      this.abortController = new AbortController();
      this.bluetoothDevice.addEventListener('advertisementreceived', this.handleAdvertisement);
      await this.bluetoothDevice.watchAdvertisements({ signal: this.abortController.signal });
      
      this.zone.run(() => {
        this.foundDevice.set({
          name: this.bluetoothDevice?.name ?? 'Amigo Desconhecido',
          id: this.bluetoothDevice?.id ?? 'N/A',
        });
        this.connectionStatus.set('watching');
        this.error.set(`Observando ${this.foundDevice()?.name}. Aproxime-se para conectar.`);
        this.isLoading.set(false);
        this.isWatching.set(true);
      });

    } catch (error: any) {
      this.handleError(error);
    }
  }

  async connect(): Promise<void> {
    if (!this.bluetoothDevice) return;
    
    this.stopScan();

    this.zone.run(() => {
        this.isLoading.set(true);
        this.connectionStatus.set('connecting');
        this.error.set(`Conectando ao ${this.foundDevice()?.name}...`);
    });

    try {
      this.bluetoothDevice.addEventListener('gattserverdisconnected', this.onDisconnected);
      const server = await this.bluetoothDevice.gatt!.connect();

      const nusService = await server.getPrimaryService(NUS_SERVICE_UUID);
      this.nusRxCharacteristic = await nusService.getCharacteristic(NUS_RX_CHARACTERISTIC_UUID);
      this.nusTxCharacteristic = await nusService.getCharacteristic(NUS_TX_CHARACTERISTIC_UUID);
      await this.nusTxCharacteristic.startNotifications();
      this.nusTxCharacteristic.addEventListener('characteristicvaluechanged', this.handleNusNotifications);

      try {
        const batteryService = await server.getPrimaryService(BATTERY_SERVICE_UUID);
        this.batteryLevelCharacteristic = await batteryService.getCharacteristic(BATTERY_LEVEL_CHARACTERISTIC_UUID);
        await this.batteryLevelCharacteristic.startNotifications();
        this.batteryLevelCharacteristic.addEventListener('characteristicvaluechanged', this.handleBatteryLevelNotifications);
        const batteryData = await this.batteryLevelCharacteristic.readValue();
        this.updateBatteryLevel(batteryData);
      } catch (e) {
         console.warn('Serviço de Bateria não encontrado.', e);
      }
      
      this.zone.run(() => {
        this.connectedDevice.set(this.foundDevice());
        this.connectionStatus.set('connected');
        this.error.set(`Conectado com sucesso ao ${this.connectedDevice()?.name}.`);
        this.lastResponse.set('Conexão estabelecida.');
        this.isLoading.set(false);
        this.foundDevice.set(null);
        this.rssi.set(null);
      });

    } catch (error: any) {
      this.handleError(error);
    }
  }

  disconnect(): void {
    if (this.bluetoothDevice?.gatt?.connected) {
      this.bluetoothDevice.gatt.disconnect();
    } else {
      this.onDisconnected();
    }
  }

  private stopScan() {
    if (this.abortController) {
      this.abortController.abort();
      this.abortController = null;
    }
    if (this.bluetoothDevice) {
      this.bluetoothDevice.removeEventListener('advertisementreceived', this.handleAdvertisement);
    }
    this.isWatching.set(false);
  }
  
  private playAlertSound() {
    if (!this.audioContext || this.audioContext.state === 'suspended') {
      this.audioContext?.resume();
    }
    if (this.audioContext) {
      const oscillator = this.audioContext.createOscillator();
      const gainNode = this.audioContext.createGain();
      oscillator.connect(gainNode);
      gainNode.connect(this.audioContext.destination);
      oscillator.type = 'sine';
      oscillator.frequency.setValueAtTime(440, this.audioContext.currentTime);
      gainNode.gain.setValueAtTime(0.5, this.audioContext.currentTime);
      oscillator.start();
      oscillator.stop(this.audioContext.currentTime + 0.1);
    }
  }

  private handleAdvertisement = (event: any) => {
    this.zone.run(() => {
        if (event.rssi) {
            this.rssi.set(event.rssi);
            if (this.proximity() === 'longe') {
                this.playAlertSound();
            }
        }
    });
  }
  
  async sendRawCommand(command: string): Promise<void> {
    if (this.connectionStatus() !== 'connected' || !this.nusRxCharacteristic) {
      this.error.set('Não é possível enviar comando: Amigo não conectado.');
      return;
    }
    try {
      const data = this.textEncoder.encode(command);
      await this.nusRxCharacteristic.writeValueWithoutResponse(data);
    } catch (error: any) {
      this.error.set(`Erro ao enviar comando: ${error.message}`);
    }
  }

  rebootDevice(): Promise<void> {
    return this.sendRawCommand('r');
  }

  requestBatteryLevelUpdate(): Promise<void> {
    return this.sendRawCommand('p');
  }

  toggleBuzzer(isOn: boolean): Promise<void> {
      return this.sendRawCommand(isOn ? 'b' : 'e');
  }

  toggleLed(isOn: boolean): Promise<void> {
      return this.sendRawCommand(isOn ? 'l' : 'o');
  }

  private handleNusNotifications = (event: Event) => {
    const characteristic = event.target as BluetoothRemoteGATTCharacteristic;
    const value = characteristic.value;
    if (value) {
      const message = this.textDecoder.decode(value).trim();
      this.zone.run(() => {
        this.lastResponse.set(message);
      });
    }
  };

  private handleBatteryLevelNotifications = (event: Event) => {
    const characteristic = event.target as BluetoothRemoteGATTCharacteristic;
    if (characteristic.value) {
      this.updateBatteryLevel(characteristic.value);
    }
  }
  
  private updateBatteryLevel(dataView: DataView) {
      const level = dataView.getUint8(0);
      this.zone.run(() => {
        this.batteryLevel.set(level);
      });
  }

  private onDisconnected = () => {
    this.zone.run(() => {
      this.stopScan();
      this.error.set('Amigo desconectado. Pronto para uma nova busca.');
      
      if (this.nusTxCharacteristic) {
        this.nusTxCharacteristic.removeEventListener('characteristicvaluechanged', this.handleNusNotifications);
      }
      if (this.batteryLevelCharacteristic) {
        this.batteryLevelCharacteristic.removeEventListener('characteristicvaluechanged', this.handleBatteryLevelNotifications);
      }
      if(this.bluetoothDevice) {
        this.bluetoothDevice.removeEventListener('gattserverdisconnected', this.onDisconnected);
      }

      this.bluetoothDevice = null;
      this.nusRxCharacteristic = null;
      this.nusTxCharacteristic = null;
      this.batteryLevelCharacteristic = null;
      
      this.connectedDevice.set(null);
      this.foundDevice.set(null);
      this.connectionStatus.set('disconnected');
      this.lastResponse.set(null);
      this.batteryLevel.set(null);
      this.rssi.set(null);
      this.isLoading.set(false);
    });
  }
  
  private handleError(error: any) {
    this.zone.run(() => {
      if (error.name !== 'AbortError' && error.name !== 'NotFoundError') {
        this.error.set(`Erro: ${error.message}`);
      }
      this.onDisconnected();
    });
  }
}
