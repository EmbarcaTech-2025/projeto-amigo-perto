import { ChangeDetectionStrategy, Component, effect, inject, Renderer2, OnDestroy } from '@angular/core';
import { HeaderComponent } from './header/header.component';
import { DeviceScannerComponent } from './device-scanner/device-scanner.component';
import { DeviceDetailsComponent } from './device-details/device-details.component';
import { BluetoothService } from './bluetooth.service';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-root',
  template: `
    <app-header />
    <main>
      <app-device-scanner />
      @if (bluetoothService.error(); as errorMessage) {
        <div class="status-container">{{ errorMessage }}</div>
      }
      <app-device-details [device]="bluetoothService.device()" />
    </main>
  `,
  styles: [`
    main {
      padding: 20px;
      max-width: 800px;
      margin: 0 auto;
      text-align: center;
    }
    .status-container {
      margin: 20px 0;
      font-style: italic;
      color: #555;
    }
  `],
  imports: [CommonModule, HeaderComponent, DeviceScannerComponent, DeviceDetailsComponent],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class AppComponent implements OnDestroy {
  public bluetoothService = inject(BluetoothService);
  private renderer = inject(Renderer2);

  private alertInterval: any;
  private isAlerting = false; // Novo estado para controlar o ciclo de alerta

  constructor() {
    // O effect agora só dispara o alerta, mas não o cancela.
    effect(() => {
      if (this.bluetoothService.isOutOfRange() && !this.isAlerting) {
        this.triggerAlert();
      }
    });
  }

  triggerAlert() {
    if (this.isAlerting) return; // Se já está alertando, não faz nada

    this.isAlerting = true; // Trava o estado de alerta
    let alertCount = 0;

    this.alertInterval = setInterval(() => {
      if (alertCount < 5) {
        this.renderer.addClass(document.body, 'alert-active');
        this.bluetoothService.beep();
        setTimeout(() => {
          this.renderer.removeClass(document.body, 'alert-active');
        }, 250); // Duração do pisca-pisca visual
        alertCount++;
      } else {
        // Ciclo de 5 alertas concluído. Limpa o intervalo e destrava o estado.
        if (this.alertInterval) {
          clearInterval(this.alertInterval);
          this.alertInterval = null;
        }
        this.isAlerting = false; // Libera para um novo ciclo de alerta, se necessário
      }
    }, 500); // Intervalo entre os beeps/piscadas
  }

  ngOnDestroy(): void {
    // Garante que o intervalo seja limpo se o componente for destruído
    if (this.alertInterval) {
      clearInterval(this.alertInterval);
    }
    this.renderer.removeClass(document.body, 'alert-active');
  }
}
