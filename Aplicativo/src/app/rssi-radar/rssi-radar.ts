
import {
  ChangeDetectionStrategy,
  Component,
  computed,
  effect,
  input,
  signal,
  OnDestroy,
} from '@angular/core';
import { LowerCasePipe } from '@angular/common';

const RSSI_MIN = -100; // Sinal mais fraco
const RSSI_MAX = -30;  // Sinal mais forte

export type Proximity = 'PERTO' | 'MEDIO' | 'LONGE';

export interface RadarOutput {
  raw: number;
  filtered: number;
  state: Proximity;
  changed: boolean;
}

export interface EwmaHysteresisConfig {
  rssiMinDbm: number;
  rssiMaxDbm: number;
  spikeRejectDb: number;
  allowSpikeReject: boolean;
  alpha: number;
  nearEnterDbm: number;
  nearExitDbm: number;
  farEnterDbm: number;
  farExitDbm: number;
  minDwellMs: number;
}

function clamp(x: number, lo: number, hi: number) {
  return Math.max(lo, Math.min(hi, x));
}

export class EwmaHysteresisRadar {
  private smoothed: number | null = null;
  private lastRaw: number | null = null;
  private state: Proximity = 'MEDIO';
  private pending: { candidate: Proximity; sinceMs: number } | null = null;

  constructor(private readonly cfg: EwmaHysteresisConfig) {}

  push(rawRssiDbm: number, nowMs: number = performance.now()): RadarOutput | null {
    const raw = clamp(rawRssiDbm, this.cfg.rssiMinDbm, this.cfg.rssiMaxDbm);

    if (this.cfg.allowSpikeReject && this.lastRaw !== null) {
      const delta = Math.abs(raw - this.lastRaw);
      if (delta >= this.cfg.spikeRejectDb) {
        this.lastRaw = raw;
        return null;
      }
    }
    this.lastRaw = raw;

    if (this.smoothed === null) this.smoothed = raw;
    else this.smoothed = this.cfg.alpha * raw + (1 - this.cfg.alpha) * this.smoothed;

    const filtered = this.smoothed;
    const candidate = this.classifyWithHysteresis(filtered);

    let changed = false;
    if (candidate !== this.state) {
      if (!this.pending || this.pending.candidate !== candidate) {
        this.pending = { candidate, sinceMs: nowMs };
      } else {
        if (nowMs - this.pending.sinceMs >= this.cfg.minDwellMs) {
          this.state = candidate;
          this.pending = null;
          changed = true;
        }
      }
    } else {
      this.pending = null;
    }

    return { raw, filtered, state: this.state, changed };
  }

  getState(): Proximity {
    return this.state;
  }

  private classifyWithHysteresis(rssi: number): Proximity {
    const { nearEnterDbm, nearExitDbm, farEnterDbm, farExitDbm } = this.cfg;

    switch (this.state) {
      case 'PERTO':
        if (rssi < nearExitDbm) return 'MEDIO';
        return 'PERTO';

      case 'LONGE':
        if (rssi > farExitDbm) return 'MEDIO';
        return 'LONGE';

      case 'MEDIO':
      default:
        if (rssi >= nearEnterDbm) return 'PERTO';
        if (rssi <= farEnterDbm) return 'LONGE';
        return 'MEDIO';
    }
  }
}

export function createDefaultEwmaRadar(): EwmaHysteresisRadar {
  return new EwmaHysteresisRadar({
    rssiMinDbm: -100,
    rssiMaxDbm: -20,
    spikeRejectDb: 12,
    allowSpikeReject: true,
    alpha: 0.2,
    nearEnterDbm: -62,
    nearExitDbm: -66,
    farEnterDbm: -82,
    farExitDbm: -78,
    minDwellMs: 1200,
  });
}

@Component({
  selector: 'app-rssi-radar',
  templateUrl: './rssi-radar.html',
  styleUrls: ['./rssi-radar.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [LowerCasePipe],
})
export class RssiRadarComponent implements OnDestroy {
  public rssi = input.required<number | null>();

  private radar = createDefaultEwmaRadar();
  private radarInterval: any = null;

  public filteredRssi = signal<number | null>(null);
  public proximidade = signal<Proximity>('MEDIO');

  public dotStyle = computed(() => {
    const currentRssi = this.filteredRssi();
    if (currentRssi === null) {
      return { transform: 'translate(0, 0)' };
    }
    const normalized = Math.max(0, Math.min(1, (currentRssi - RSSI_MIN) / (RSSI_MAX - RSSI_MIN)));
    const angle = (1 - normalized) * 1.5 * Math.PI - 0.75 * Math.PI;
    const radius = 120;
    const x = Math.cos(angle) * radius;
    const y = Math.sin(angle) * radius;
    return { transform: `translate(${x}px, ${y}px)` };
  });

  private audioContext: AudioContext | null = null;
  private oscillator: OscillatorNode | null = null;
  private gainNode: GainNode | null = null;
  private soundInterval: any = null;

  constructor() {
    effect(() => {
      const currentRssi = this.rssi();
      if (currentRssi !== null) {
        const output = this.radar.push(currentRssi);
        if (output) {
          this.filteredRssi.set(output.filtered);
          if (output.changed) {
            this.proximidade.set(output.state);
          }
        }
      }
    });

    effect(() => {
      if (this.proximidade() === 'LONGE') {
        this.playWarningSound();
      } else {
        this.stopWarningSound();
      }
    });
  }

  ngOnDestroy(): void {
    this.stopWarningSound();
    if (this.radarInterval) {
      clearInterval(this.radarInterval);
    }
  }

  private playWarningSound(): void {
    if (!this.audioContext) {
      this.audioContext = new (window.AudioContext || (window as any).webkitAudioContext)();
    }
    if (this.soundInterval) return;

    this.soundInterval = setInterval(() => {
      if (!this.audioContext) return;
      this.oscillator = this.audioContext.createOscillator();
      this.gainNode = this.audioContext.createGain();

      this.oscillator.connect(this.gainNode);
      this.gainNode.connect(this.audioContext.destination);

      this.oscillator.type = 'sine';
      this.oscillator.frequency.setValueAtTime(880, this.audioContext.currentTime);
      this.gainNode.gain.setValueAtTime(0.3, this.audioContext.currentTime);
      this.gainNode.gain.exponentialRampToValueAtTime(
        0.00001,
        this.audioContext.currentTime + 0.2
      );

      this.oscillator.start(this.audioContext.currentTime);
      this.oscillator.stop(this.audioContext.currentTime + 0.2);
    }, 1000);
  }

  private stopWarningSound(): void {
    if (this.soundInterval) {
      clearInterval(this.soundInterval);
      this.soundInterval = null;
    }
    if (this.oscillator) {
      this.oscillator.stop();
      this.oscillator = null;
    }
  }
}
