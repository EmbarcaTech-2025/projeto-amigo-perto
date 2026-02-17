
import { Injectable, signal } from '@angular/core';

// UUIDs para o serviço personalizado do Amigo Perto
const CUSTOM_SERVICE_UUID = '12345678-abcd-efab-cdef-123456789abc';
const CUSTOM_RX_CHARACTERISTIC_UUID = '12345679-abcd-efab-cdef-123456789abc';

// UUIDs para o Serviço de Bateria padrão do Bluetooth
const BATTERY_SERVICE_UUID = '0000180f-0000-1000-8000-00805f9b34fb'; // UUID completo
const BATTERY_LEVEL_CHARACTERISTIC_UUID = '00002a19-0000-1000-8000-00805f9b34fb';

@Injectable({ providedIn: 'root' })
export class BluetoothService {
  private device: BluetoothDevice | null = null;
  private server: BluetoothRemoteGATTServer | null = null;
  private rxCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private abortController: AbortController | null = null;

  // --- NOVO: Timer para polling da bateria ---
  private batteryPollInterval: any = null;

  // Sinais de estado
  public isConnected = signal(false);
  public isConnecting = signal(false);
  public isScanning = signal(false);
  public rssi = signal<number | null>(null);
  public error = signal<string | null>(null);
  public batteryLevel = signal<number | null>(null);

  async startScan() {
    this.error.set(null);
    this.isScanning.set(true);
    this.rssi.set(null);

    try {
      this.abortController = new AbortController();
      this.device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [CUSTOM_SERVICE_UUID] }],
        // Solicita acesso opcional ao serviço de bateria
        optionalServices: [CUSTOM_SERVICE_UUID, BATTERY_SERVICE_UUID],
      });

      if (!this.device) { throw new Error('Nenhum dispositivo selecionado.'); }

      this.device.addEventListener('advertisementreceived', (event: any) => this.rssi.set(event.rssi));
      await this.device.watchAdvertisements({ signal: this.abortController.signal });

    } catch (e: any) {
      if (e.name !== 'NotFoundError') { this.error.set(`Erro ao escanear: ${e.message}`); }
      this.isScanning.set(false);
    }
  }

  async connectToDevice() {
    if (!this.device) { this.error.set('Nenhum dispositivo para conectar.'); return; }

    this.isConnecting.set(true);
    this.isScanning.set(false);
    this.error.set(null);
    this.abortController?.abort();

    try {
      this.device.addEventListener('gattserverdisconnected', () => this.handleDisconnect());
      this.server = await this.device.gatt?.connect() ?? null;
      
      const customService = await this.server?.getPrimaryService(CUSTOM_SERVICE_UUID);
      this.rxCharacteristic = await customService?.getCharacteristic(CUSTOM_RX_CHARACTERISTIC_UUID) ?? null;

      // --- ATUALIZADO: Inicia a leitura e o polling da bateria ---
      await this.setupBatteryPolling();

      this.isConnected.set(true);
    } catch (e: any) {
      this.error.set(`Erro ao conectar: ${e.message}`);
    } finally {
      this.isConnecting.set(false);
    }
  }

  // --- ATUALIZADO: Configura a leitura inicial e o polling periódico ---
  private async setupBatteryPolling() {
    // 1. Faz a leitura inicial imediatamente após a conexão
    await this.readBatteryLevel();

    // 2. Configura um timer para ler a bateria a cada 30 segundos
    this.batteryPollInterval = setInterval(() => {
      this.readBatteryLevel();
    }, 30000); // 30000 ms = 30 segundos
  }

  // --- NOVO: Função que lê ativamente o nível da bateria ---
  public async readBatteryLevel() {
    if (!this.server?.connected) {
      console.warn('Tentativa de ler bateria sem conexão.');
      return;
    }
    
    try {
      const batteryService = await this.server.getPrimaryService(BATTERY_SERVICE_UUID);
      const characteristic = await batteryService.getCharacteristic(BATTERY_LEVEL_CHARACTERISTIC_UUID);
      const value = await characteristic.readValue();
      const level = value.getUint8(0);
      this.batteryLevel.set(level);
      console.log(`Nível da bateria lido: ${level}%`);

    } catch (e) {
      // Limpa o valor se a leitura falhar (ex: serviço não encontrado)
      this.batteryLevel.set(null);
      console.warn('Não foi possível ler o nível da bateria.', e);
      // Para o polling se o serviço não for encontrado para evitar erros repetidos
      if (this.batteryPollInterval) {
        clearInterval(this.batteryPollInterval);
        this.batteryPollInterval = null;
      }
    }
  }

  async sendCommand(command: number) {
    if (!this.rxCharacteristic) { this.error.set('Característica de escrita não encontrada.'); return; }
    const buffer = new Uint8Array([command]);
    await this.rxCharacteristic.writeValue(buffer);
  }

  disconnect() {
    if (this.isScanning()) {
      this.abortController?.abort();
      this.isScanning.set(false);
      return;
    }
    if (this.server?.connected) { this.server.disconnect(); }
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

    // --- ATUALIZADO: Limpa o timer e o valor da bateria ao desconectar ---
    if (this.batteryPollInterval) {
      clearInterval(this.batteryPollInterval);
      this.batteryPollInterval = null;
    }
    this.batteryLevel.set(null);
    console.log('Dispositivo desconectado e polling de bateria interrompido.');
  }
}
