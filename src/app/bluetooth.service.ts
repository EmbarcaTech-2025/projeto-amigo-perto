
import { Injectable, signal, inject, NgZone } from '@angular/core';

// --- Type Definitions for Web Bluetooth API ---
interface BluetoothDevice extends EventTarget {
  readonly id: string;
  readonly name?: string | undefined;
  readonly gatt?: BluetoothRemoteGATTServer | undefined;
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
const NUS_RX_CHARACTERISTIC_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // App -> Device (Write)
const NUS_TX_CHARACTERISTIC_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Device -> App (Notify)

const BATTERY_SERVICE_UUID = '0000180f-0000-1000-8000-00805f9b34fb';
const BATTERY_LEVEL_CHARACTERISTIC_UUID = '00002a19-0000-1000-8000-00805f9b34fb';


export interface Device {
  name: string;
  id: string;
}

export type ConnectionStatus = 'disconnected' | 'searching' | 'connecting' | 'connected';

@Injectable({
  providedIn: 'root',
})
export class BluetoothService {
  private zone = inject(NgZone);
  private textEncoder = new TextEncoder();
  private textDecoder = new TextDecoder();
  
  // --- Bluetooth State ---
  private bluetoothDevice: BluetoothDevice | null = null;
  private nusRxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private nusTxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private batteryLevelCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  // --- Public State Signals ---
  device = signal<Device | null>(null);
  connectionStatus = signal<ConnectionStatus>('disconnected');
  error = signal<string | null>('Pronto para iniciar. Clique em "Conectar" para procurar um dispositivo.');
  lastResponse = signal<string | null>(null);
  batteryLevel = signal<number | null>(null);
  isLoading = signal<boolean>(false);

  // --- Public Actions ---
  
  async findAndConnect(): Promise<void> {
    if (this.connectionStatus() !== 'disconnected') return;
    if (!navigator.bluetooth) {
      this.error.set('Web Bluetooth não é suportado neste navegador.');
      return;
    }

    this.zone.run(() => {
        this.isLoading.set(true);
        this.connectionStatus.set('searching');
        this.error.set('Procurando dispositivo... Por favor, selecione-o na janela.');
    });

    try {
      this.bluetoothDevice = await navigator.bluetooth.requestDevice({
        filters: [{ services: [NUS_SERVICE_UUID] }],
        optionalServices: [BATTERY_SERVICE_UUID]
      });

      this.zone.run(() => {
        this.device.set({
          name: this.bluetoothDevice?.name ?? 'Dispositivo Desconhecido',
          id: this.bluetoothDevice?.id ?? 'N/A',
        });
        this.connectionStatus.set('connecting');
        this.error.set(`Conectando ao ${this.device()?.name}...`);
      });

      this.bluetoothDevice.addEventListener('gattserverdisconnected', this.onDisconnected);
      const server = await this.bluetoothDevice.gatt!.connect();

      // --- NUS Setup ---
      const nusService = await server.getPrimaryService(NUS_SERVICE_UUID);
      this.nusRxCharacteristic = await nusService.getCharacteristic(NUS_RX_CHARACTERISTIC_UUID);
      this.nusTxCharacteristic = await nusService.getCharacteristic(NUS_TX_CHARACTERISTIC_UUID);
      await this.nusTxCharacteristic.startNotifications();
      this.nusTxCharacteristic.addEventListener('characteristicvaluechanged', this.handleNusNotifications);

      // --- Battery Service (BAS) Setup ---
      try {
        const batteryService = await server.getPrimaryService(BATTERY_SERVICE_UUID);
        this.batteryLevelCharacteristic = await batteryService.getCharacteristic(BATTERY_LEVEL_CHARACTERISTIC_UUID);
        await this.batteryLevelCharacteristic.startNotifications();
        this.batteryLevelCharacteristic.addEventListener('characteristicvaluechanged', this.handleBatteryLevelNotifications);
        // Read initial value
        const batteryData = await this.batteryLevelCharacteristic.readValue();
        this.updateBatteryLevel(batteryData);
      } catch (e) {
         console.warn('Serviço de Bateria não encontrado. Funcionalidade de bateria desabilitada.', e);
      }
      
      this.zone.run(() => {
        this.connectionStatus.set('connected');
        this.error.set(`Conectado com sucesso ao ${this.device()?.name}.`);
        this.lastResponse.set('Conexão estabelecida.');
        this.isLoading.set(false);
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

  async sendRawCommand(command: string): Promise<void> {
    if (this.connectionStatus() !== 'connected' || !this.nusRxCharacteristic) {
      this.error.set('Não é possível enviar comando: dispositivo não conectado.');
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


  // --- Private Handlers and Methods ---

  private handleNusNotifications = (event: Event) => {
    const characteristic = event.target as BluetoothRemoteGATTCharacteristic;
    const value = characteristic.value;
    if (value) {
      const message = this.textDecoder.decode(value).trim();
      this.zone.run(() => {
        this.lastResponse.set(message);
        console.log('NUS RX:', message);
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
        console.log(`Nível da Bateria: ${level}%`);
      });
  }

  private onDisconnected = () => {
    this.zone.run(() => {
      this.error.set('Dispositivo desconectado. Pronto para uma nova busca.');
      
      // --- Clean up listeners ---
      if (this.nusTxCharacteristic) {
        this.nusTxCharacteristic.removeEventListener('characteristicvaluechanged', this.handleNusNotifications);
      }
      if (this.batteryLevelCharacteristic) {
        this.batteryLevelCharacteristic.removeEventListener('characteristicvaluechanged', this.handleBatteryLevelNotifications);
      }
      if(this.bluetoothDevice) {
        this.bluetoothDevice.removeEventListener('gattserverdisconnected', this.onDisconnected);
      }

      // --- Reset state ---
      this.bluetoothDevice = null;
      this.nusRxCharacteristic = null;
      this.nusTxCharacteristic = null;
      this.batteryLevelCharacteristic = null;
      
      this.device.set(null);
      this.connectionStatus.set('disconnected');
      this.lastResponse.set(null);
      this.batteryLevel.set(null);
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
