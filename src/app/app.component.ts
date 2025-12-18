import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { BluetoothService } from './bluetooth.service';
import { DeviceInfoComponent } from './device-info.component';
import { RssiRadarComponent } from './rssi-radar.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [
    CommonModule, 
    DeviceInfoComponent,
    RssiRadarComponent
  ]
})
export class AppComponent {
  public bluetoothService = inject(BluetoothService);
}
