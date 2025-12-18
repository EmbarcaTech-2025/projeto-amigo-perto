import { Injectable, signal } from '@angular/core';

// FIX: UUIDs atualizados com base no feedback do dispositivo do usuário.
const CUSTOM_SERVICE_UUID = '12345678-abcd-efab-cdef-123456789abc';
const CUSTOM_RX_CHARACTERISTIC_UUID = '12345679-abcd-efab-cdef-123456789abc';

@Injectable({ providedIn: 'root' })
export class BluetoothService {
  private device: BluetoothDevice | null = null;
  private server: BluetoothRemoteGATTServer | null = null;
  // A característica TX não é necessária, pois apenas enviamos comandos (RX).
  private rxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private abortController: AbortController | null = null;

  public isConnected = signal(false);
  public isConnecting = signal(false);
  public isScanning = signal(false);
  public rssi = signal<number | null>(null);
  public error = signal<string | null>(null);

  async startScan() {
    this.error.set(null);
    this.isScanning.set(true);
    this.rssi.set(null);

    try {
      this.abortController = new AbortController();

      // FIX: Filtro reativado com o Service UUID correto. A busca volta a ser automática.
      this.device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [CUSTOM_SERVICE_UUID] }],
        optionalServices: [CUSTOM_SERVICE_UUID],
      });

      if (!this.device) {
        throw new Error('Nenhum dispositivo selecionado.');
      }

      this.device.addEventListener('advertisementreceived', (event: any) => {
        this.rssi.set(event.rssi);
      });

      await this.device.watchAdvertisements({ signal: this.abortController.signal });

    } catch (e: any) {
      console.error(e);
      if (e.name !== 'NotFoundError') {
        this.error.set(`Erro ao escanear: ${e.message}`);
      }
      this.isScanning.set(false);
    }
  }

  async connectToDevice() {
    if (!this.device) {
      this.error.set('Nenhum dispositivo para conectar.');
      return;
    }

    this.isConnecting.set(true);
    this.isScanning.set(false);
    this.error.set(null);

    this.abortController?.abort();

    try {
      this.device.addEventListener('gattserverdisconnected', () => this.handleDisconnect());
      this.server = await this.device.gatt?.connect() ?? null;
      const service = await this.server?.getPrimaryService(CUSTOM_SERVICE_UUID);
      // FIX: Procura pela característica correta de escrita (RX do ponto de vista do app).
      this.rxCharacteristic = await service?.getCharacteristic(CUSTOM_RX_CHARACTERISTIC_UUID) ?? null;

      this.isConnected.set(true);
    } catch (e: any) {
      console.error(e);
      this.error.set(`Erro ao conectar: ${e.message}`);
    } finally {
      this.isConnecting.set(false);
    }
  }

  async sendCommand(command: string) {
    if (!this.rxCharacteristic) {
      this.error.set('Característica de escrita não encontrada.');
      return;
    }
    const encoder = new TextEncoder();
    await this.rxCharacteristic.writeValue(encoder.encode(command + '\n'));
  }

  disconnect() {
    if (this.isScanning()) {
      this.abortController?.abort();
      this.isScanning.set(false);
      console.log('Escaneamento cancelado.');
      return;
    }

    if (this.server?.connected) {
      this.server.disconnect();
    }
  }

  private handleDisconnect() {
    this.isConnected.set(false);
    this.isConnecting.set(false);
    this.isScanning.set(false);
    this.rssi.set(null);
    this.device = null;
    this.server = null;
    this.rxCharacteristic = null;
    this.abortController = null;
    console.log('Dispositivo desconectado.');
  }
}
