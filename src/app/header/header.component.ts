
import { ChangeDetectionStrategy, Component } from '@angular/core';

@Component({
  selector: 'app-header',
  template: `
    <header>
      <img src="assets/amigoperto.png" alt="AmigoPerto Logo">
      <h1>AmigoPerto</h1>
      <p>Encontre seus amigos por perto.</p>
    </header>
  `,
  styles: [`
    header {
      text-align: center;
      margin-bottom: 40px;
    }

    img {
      width: 120px;
      height: 120px;
      margin-bottom: 15px;
      border-radius: 50%;
      object-fit: cover;
    }

    h1 {
      margin: 0;
      font-size: 3rem;
      font-weight: 600;
      color: #333;
    }

    p {
      margin: 5px 0 0;
      color: #777;
      font-size: 1.2rem;
    }
  `],
  changeDetection: ChangeDetectionStrategy.OnPush
})
export class HeaderComponent {}
