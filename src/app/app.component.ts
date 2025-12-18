import { ChangeDetectionStrategy, Component } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ControlPadComponent } from './control-pad/control-pad.component';
import { BluetoothService } from './bluetooth.service';
import { RssiRadarComponent } from './rssi-radar/rssi-radar';

@Component({
  selector: 'app-root',
  imports: [
    CommonModule,
    ControlPadComponent,
    RssiRadarComponent
  ],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class AppComponent {
  // A injeção do serviço permanece a mesma.
  constructor(public bluetoothService: BluetoothService) {}
}
