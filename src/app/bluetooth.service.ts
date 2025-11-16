import { Injectable, signal, inject, NgZone } from '@angular/core';

const TICO_DEVICE_ID = 'rrZ4Kal58RvIdNCLoOshUg==';
const TICO_PHOTO_URL = '/assets/tico.png';

// A interface Device DEVE ter a propriedade photoUrl
export interface Device {
  name: string;
  id: string;
  rssi?: number;
  photoUrl?: string;
}

@Injectable({
  providedIn: 'root',
})
export class BluetoothService {
  private zone = inject(NgZone);

  devices = signal<Device[]>([]);
  selectedDevice = signal<Device | null>(null);
  scanning = signal<boolean>(false);
  error = signal<string | null>(null);

  async scan(): Promise<void> {
    if (!navigator.bluetooth) {
      this.error.set('A API de Web Bluetooth não é suportada neste navegador.');
      return;
    }

    this.scanning.set(true);
    this.error.set(null);

    try {
      const device = await navigator.bluetooth.requestDevice({
        acceptAllDevices: true,
      });

      const newDevice: Device = {
        name: device.name ?? 'Dispositivo Desconhecido',
        id: device.id,
      };

      // Se for o Tico, atribui a URL da foto
      if (device.id === TICO_DEVICE_ID) {
        newDevice.photoUrl = TICO_PHOTO_URL;
      }

      this.zone.run(() => {
        this.devices.update(devices => {
          if (!devices.some(d => d.id === newDevice.id)) {
            return [...devices, newDevice];
          }
          return devices;
        });
        this.selectedDevice.set(newDevice);
      });

      this.watchAdvertisements(device);

    } catch (error: any) {
      this.zone.run(() => {
        if (error.name !== 'NotFoundError') {
          this.error.set(`Erro: ${error.message}`);
        }
      });
    } finally {
      this.zone.run(() => {
        this.scanning.set(false);
      });
    }
  }

  private watchAdvertisements(device: BluetoothDevice) {
    device.addEventListener('advertisementreceived', (event: any) => {
      this.zone.run(() => {
        const updatedRssi = { rssi: event.rssi };

        if (this.selectedDevice()?.id === device.id) {
          this.selectedDevice.update(currentDevice => 
            currentDevice ? { ...currentDevice, ...updatedRssi } : null
          );
        }

        this.devices.update(devices => 
          devices.map(d => d.id === device.id ? { ...d, ...updatedRssi } : d)
        );
      });
    });

    device.watchAdvertisements();
    console.log('Observando anúncios para', device.name);
  }
}
