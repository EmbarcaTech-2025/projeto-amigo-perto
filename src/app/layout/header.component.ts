
import { ChangeDetectionStrategy, Component, signal } from '@angular/core';

@Component({
  selector: 'app-header',
  template: `
    <header>
      <img [src]="mascot()" alt="Mascote do AmigoPerto" class="mascot"/>
      <h1>AmigoPerto</h1>
      <p>Encontre seus amigos por perto.</p>
    </header>
  `,
  styles: `
    header {
      text-align: center;
      padding: 2rem 1rem;
      background-color: #f8f9fa;
      border-bottom: 1px solid #dee2e6;
    }

    .mascot {
      width: 100px;
      height: 100px;
      border-radius: 50%;
      object-fit: cover;
      margin-bottom: 1rem;
      border: 4px solid #fff;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    }

    h1 {
      font-size: 2.5rem;
      font-weight: 700;
      color: #343a40;
      margin: 0;
    }

    p {
      font-size: 1.1rem;
      color: #6c757d;
    }
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class HeaderComponent {
  mascot = signal('assets/tico.png');
}


