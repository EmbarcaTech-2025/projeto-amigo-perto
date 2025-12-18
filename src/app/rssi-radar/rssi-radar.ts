import {
  ChangeDetectionStrategy,
  Component,
  computed,
  effect,
  input,
  signal,
} from '@angular/core';
import { LowerCasePipe } from '@angular/common';

const RSSI_MIN = -100; // Sinal mais fraco
const RSSI_MAX = -30;  // Sinal mais forte

type Proximidade = 'Perto' | 'Média' | 'Longe';

@Component({
  selector: 'app-rssi-radar',
  templateUrl: './rssi-radar.html',
  styleUrls: ['./rssi-radar.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
  imports: [LowerCasePipe] // Importa o pipe
})
export class RssiRadarComponent {
  public rssi = input.required<number | null>();

  // Torna a propriedade pública
  public proximidade = computed<Proximidade>(() => {
    const currentRssi = this.rssi();
    if (currentRssi === null) return 'Longe';
    if (currentRssi > -60) return 'Perto';
    if (currentRssi > -80) return 'Média';
    return 'Longe';
  });

  public dotStyle = computed(() => {
    const currentRssi = this.rssi();
    if (currentRssi === null) {
      return { transform: 'translate(0, 0)' };
    }
    const normalized = Math.max(0, Math.min(1, (currentRssi - RSSI_MIN) / (RSSI_MAX - RSSI_MIN)));
    const angle = (1 - normalized) * 1.5 * Math.PI - (0.75 * Math.PI); // Ajuste para o arco superior
    const radius = 120;
    const x = Math.cos(angle) * radius;
    const y = Math.sin(angle) * radius;
    return { transform: `translate(${x}px, ${y}px)` };
  });

  // --- LÓGICA DE ÁUDIO --- 
  private audioContext: AudioContext | null = null;
  private oscillator: OscillatorNode | null = null;
  private gainNode: GainNode | null = null;
  private soundInterval: any = null;

  constructor() {
    effect(() => {
      if (this.proximidade() === 'Longe') {
        this.playWarningSound();
      } else {
        this.stopWarningSound();
      }
    });
  }

  private playWarningSound(): void {
    if (!this.audioContext) {
      this.audioContext = new (window.AudioContext || (window as any).webkitAudioContext)();
    }
    if (this.soundInterval) return; // Já está tocando

    this.soundInterval = setInterval(() => {
      if (!this.audioContext) return;
      this.oscillator = this.audioContext.createOscillator();
      this.gainNode = this.audioContext.createGain();

      this.oscillator.connect(this.gainNode);
      this.gainNode.connect(this.audioContext.destination);

      this.oscillator.type = 'sine';
      this.oscillator.frequency.setValueAtTime(880, this.audioContext.currentTime); // La (A5)
      this.gainNode.gain.setValueAtTime(0.3, this.audioContext.currentTime);
      this.gainNode.gain.exponentialRampToValueAtTime(0.00001, this.audioContext.currentTime + 0.2);

      this.oscillator.start(this.audioContext.currentTime);
      this.oscillator.stop(this.audioContext.currentTime + 0.2);
    }, 1000); // Bipa a cada segundo
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
