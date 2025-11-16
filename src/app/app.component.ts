
import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { HeaderComponent } from './header/header.component';
import { DeviceScannerComponent } from './device-scanner/device-scanner.component';
import { DeviceDetailsComponent } from './device-details/device-details.component';
import { BluetoothService } from './bluetooth.service';

@Component({
  selector: 'app-root',
  template: `
    <app-header />
    <main>
      <app-device-scanner />
      <app-device-details />
    </main>
  `,
  styles: [`
    main {
      padding: 20px;
      max-width: 800px;
      margin: 0 auto;
    }
  `],
  imports: [HeaderComponent, DeviceScannerComponent, DeviceDetailsComponent],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class AppComponent {
  public bluetoothService = inject(BluetoothService);
}

