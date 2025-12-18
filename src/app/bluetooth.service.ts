import { Injectable, signal, WritableSignal } from '@angular/core';

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
  private leScan: BluetoothLEScan | null = null;

  // State Signals
  public readonly connectionStatus = signal<'disconnected' | 'connecting' | 'connected'>('disconnected');
  public readonly rssi: WritableSignal<number | null> = signal<number | null>(null);
  public readonly deviceInfo = signal<{ name: string; id: string } | null>(null);

  async connect() {
    try {
      if (!navigator.bluetooth) {
        throw new Error('Web Bluetooth API is not available in this browser.');
      }

      this.connectionStatus.set('connecting');

      // Request the device
      this.device = await navigator.bluetooth.requestDevice({
        acceptAllDevices: true,
        optionalServices: [NUS_SERVICE_UUID],
      });

      this.deviceInfo.set({ name: this.device.name ?? 'Unknown Device', id: this.device.id });
      this.device.addEventListener('gattserverdisconnected', this.handleDisconnection.bind(this));

      // Connect to the GATT server
      await this.device.gatt?.connect();
      this.connectionStatus.set('connected');

      // Start RSSI monitoring right after connecting
      this.startRssiMonitoring();

      // Try to get the NUS service, but don't fail if it's not there
      await this.setupNusService();

    } catch (error) {
      this.handleError(error);
    }
  }

  disconnect() {
    this.leScan?.stop();
    this.leScan = null;
    this.device?.gatt?.disconnect();
  }

  private handleDisconnection() {
    this.connectionStatus.set('disconnected');
    this.rssi.set(null);
    this.deviceInfo.set(null);
    this.leScan?.stop();
    this.leScan = null;
  }

  private async setupNusService() {
    if (!this.device?.gatt) return;

    try {
      const server = this.device.gatt;
      const service = await server.getPrimaryService(NUS_SERVICE_UUID);

      this.txCharacteristic = await service.getCharacteristic(NUS_TX_CHARACTERISTIC_UUID);
      this.rxCharacteristic = await service.getCharacteristic(NUS_RX_CHARACTERISTIC_UUID);

      await this.txCharacteristic.startNotifications();
      this.txCharacteristic.addEventListener('characteristicvaluechanged', this.handleNotifications.bind(this));
      console.log('Nordic UART Service (NUS) is set up.');
    } catch (error) {
      console.warn('Nordic UART Service (NUS) not found or failed to set up. RSSI monitoring will continue.', error);
    }
  }

  private async startRssiMonitoring() {
    if (!this.device) return;

    try {
      // Use requestLEScan to listen for advertisements
      this.leScan = await navigator.bluetooth.requestLEScan({ 
        filters: [{ services: [NUS_SERVICE_UUID] }],
       });

      navigator.bluetooth.addEventListener('advertisementreceived', (event: BluetoothAdvertisingEvent) => {
        // Check if the advertisement is from the connected device
        if (event.device.id === this.device?.id) {
          if (event.rssi) {
             this.rssi.set(event.rssi);
          }
        }
      });

    } catch (error) {
      console.error('Could not start RSSI monitoring:', error);
    }
  }

  private handleNotifications(event: Event) {
    const value = (event.target as BluetoothRemoteGATTCharacteristic).value;
    // Handle incoming data from the device
  }

  private handleError(error: unknown) {
    // Ignore user cancellation errors
    if (error instanceof Error && error.name === 'NotFoundError') {
      this.connectionStatus.set('disconnected');
      return;
    }
    console.error('Bluetooth Error:', error);
    this.disconnect();
  }
}
