import { Injectable, signal } from '@angular/core';

// UUIDs for the Nordic UART Service (NUS)
const NUS_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHARACTERISTIC_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_TX_CHARACTERISTIC_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

@Injectable({
  providedIn: 'root',
})
export class BluetoothService {
  private device: BluetoothDevice | null = null;
  private txCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private rxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  // State Signals
  public readonly connectionStatus = signal<'disconnected' | 'connecting' | 'connected'>('disconnected');
  public readonly rssi = signal<number | null>(null);
  public readonly deviceInfo = signal<{ name: string; id: string } | null>(null);

  async connect() {
    try {
      if (!navigator.bluetooth) {
        throw new Error('Web Bluetooth API is not available in this browser.');
      }

      this.connectionStatus.set('connecting');

      this.device = await navigator.bluetooth.requestDevice({
        acceptAllDevices: true, // Accept all devices
        optionalServices: [NUS_SERVICE_UUID], // Request NUS as an optional service
      });

      this.deviceInfo.set({ name: this.device.name ?? 'Unknown Device', id: this.device.id });

      const server = await this.device.gatt?.connect();
      const service = await server?.getPrimaryService(NUS_SERVICE_UUID);

      this.txCharacteristic = (await service?.getCharacteristic(NUS_TX_CHARACTERISTIC_UUID)) ?? null;
      this.rxCharacteristic = (await service?.getCharacteristic(NUS_RX_CHARACTERISTIC_UUID)) ?? null;

      // Listen for notifications
      await this.txCharacteristic?.startNotifications();
      this.txCharacteristic?.addEventListener('characteristicvaluechanged', this.handleNotifications.bind(this));
      this.device.addEventListener('gattserverdisconnected', this.handleDisconnection.bind(this));

      this.connectionStatus.set('connected');
      this.startRssiMonitoring();
    } catch (error) {
      this.handleError(error);
    }
  }

  disconnect() {
    this.device?.gatt?.disconnect();
    this.connectionStatus.set('disconnected');
    this.rssi.set(null);
    this.deviceInfo.set(null);
  }

  private async startRssiMonitoring() {
    while (this.connectionStatus() === 'connected') {
        // Note: The Web Bluetooth API does not currently support reading RSSI from a connected device.
        // This is a placeholder for future implementation or alternative approaches.
        await new Promise(resolve => setTimeout(resolve, 2000));
    }
  }

  private handleNotifications(event: Event) {
    const value = (event.target as BluetoothRemoteGATTCharacteristic).value;
    // Handle incoming data from the device
  }

  private handleDisconnection() {
    this.disconnect();
  }

  private handleError(error: unknown) {
    console.error('Bluetooth Error:', error);
    this.disconnect();
  }
}
