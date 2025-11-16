import { ChangeDetectionStrategy, Component, inject } from '@angular/core';
import { BluetoothService } from '../bluetooth.service';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-header',
  imports: [CommonModule],
  template: `
    <header>
      <div class="header-image-container">
          <img 
            id="mascot-image" 
            src="assets/amigoperto.png" 
            alt="Mascote AmigoPerto" 
            [class.alert-active]="isAlertActive()">
      </div>
      <h1>AmigoPerto</h1>
      <p>Seu amigo sempre por perto.</p>
    </header>
  `,
  styles: [`
    header {
      text-align: center;
      margin-bottom: 30px;
      width: 100%;
    }

    img {
      width: 140px; /* Aumentado */
      height: 140px; /* Aumentado */
      margin-bottom: 10px;
      border-radius: 50%;
      object-fit: cover;
      border: 4px solid var(--card-background);
      box-shadow: 0 4px 15px var(--shadow-color);
    }

    h1 {
      margin: 0;
      font-size: 3.5rem;
      font-weight: 700;
      color: var(--text-color);
      background: -webkit-linear-gradient(45deg, var(--primary-color), var(--secondary-color));
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    p {
      margin: 5px 0 0;
      color: #667;
      font-size: 1.1rem;
      font-weight: 400;
    }
  `],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class HeaderComponent {
    private bluetoothService = inject(BluetoothService);
    public isAlertActive = this.bluetoothService.isOutOfRange;
}
