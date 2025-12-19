
import { ChangeDetectionStrategy, Component } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ControlPadComponent } from './control-pad/control-pad.component';
import { BluetoothService } from './bluetooth.service';
import { RssiRadarComponent } from './rssi-radar/rssi-radar';
import { BatteryIndicatorComponent } from './features/battery-indicator/battery-indicator';

@Component({
  selector: 'app-root',
  imports: [
    CommonModule,
    ControlPadComponent,
    RssiRadarComponent,
    BatteryIndicatorComponent // <--- Adicionado aqui
  ],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class AppComponent {
  constructor(public bluetoothService: BluetoothService) {}
}
