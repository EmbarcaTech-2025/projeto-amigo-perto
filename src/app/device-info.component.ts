import { CommonModule } from '@angular/common';
import { ChangeDetectionStrategy, Component, input, output, signal } from '@angular/core';

@Component({
  selector: 'app-device-info',
  templateUrl: './device-info.component.html',
  styleUrls: ['./device-info.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [CommonModule]
})
export class DeviceInfoComponent {
  // Inputs
  public deviceName = input<string | null>(null);
  public isConnected = input(false);

  // Outputs
  public connect = output<void>();
  public disconnect = output<void>();

  // Local State
  public isBuzzerOn = signal(false);
  public isLedOn = signal(false);

  public toggleBuzzer() {
    this.isBuzzerOn.update(v => !v);
  }

  public toggleLed() {
    this.isLedOn.update(v => !v);
  }
}
