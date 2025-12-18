import { ChangeDetectionStrategy, Component, output } from '@angular/core';

@Component({
  selector: 'app-control-pad',
  templateUrl: './control-pad.component.html',
  styleUrls: ['./control-pad.component.css'],
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class ControlPadComponent {
  command = output<string>();
  disconnect = output<void>();

  onCommand(command: string) {
    this.command.emit(command);
  }
}
