import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { BluetoothService } from './bluetooth.service';
import { DeviceInfoComponent } from './device-info.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [CommonModule, NgOptimizedImage, DeviceInfoComponent] // <-- Corrigido
})
export class AppComponent {
  public bluetoothService = inject(BluetoothService);
}
