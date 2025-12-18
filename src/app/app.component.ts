import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { RssiRadarComponent } from './rssi-radar.component';
import { DeviceInfoComponent } from './device-info.component';
import { BluetoothService } from './bluetooth.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [RssiRadarComponent, DeviceInfoComponent]
})
export class AppComponent {
  public bluetoothService = inject(BluetoothService);

  onConnect() {
    this.bluetoothService.connect();
  }

  onDisconnect() {
    this.bluetoothService.disconnect();
  }
}
