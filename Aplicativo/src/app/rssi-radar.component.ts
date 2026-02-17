import { ChangeDetectionStrategy, Component, input, signal, OnDestroy, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-rssi-radar',
  templateUrl: './rssi-radar.component.html',
  styleUrls: ['./rssi-radar.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [CommonModule]
})
export class RssiRadarComponent implements OnInit, OnDestroy {
  device = input.required<BluetoothDevice>();

  rssi = signal(0);
  proximity = signal('unknown'); // e.g., 'very-close', 'close', 'far'
  proximityText = signal('Unknown'); // e.g., 'Very Close', 'Close', 'Far'

  private intervalId?: any;

  ngOnInit(): void {
    // Simulate RSSI updates
    this.intervalId = setInterval(() => {
      // Simulate a value between -100 and -30
      const simulatedRssi = Math.floor(Math.random() * 70) - 100;
      this.updateRssi(simulatedRssi);
    }, 2000);
  }

  ngOnDestroy(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
    }
  }

  private calculateProximity(rssi: number): { id: string; text: string } {
    if (rssi > -50) {
      return { id: 'proximity-very-close', text: 'Very Close' };
    } else if (rssi > -70) {
      return { id: 'proximity-close', text: 'Close' };
    } else if (rssi > -90) {
      return { id: 'proximity-far', text: 'Far' };
    } else {
      return { id: 'proximity-very-far', text: 'Very Far' };
    }
  }

  updateRssi(newRssi: number): void {
    const proximityInfo = this.calculateProximity(newRssi);
    this.rssi.set(newRssi);
    this.proximity.set(proximityInfo.id);
    this.proximityText.set(proximityInfo.text);
  }
}
