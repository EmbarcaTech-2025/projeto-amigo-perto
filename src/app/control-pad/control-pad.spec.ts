import { ComponentFixture, TestBed } from '@angular/core/testing';

import { ControlPad } from './control-pad';

describe('ControlPad', () => {
  let component: ControlPad;
  let fixture: ComponentFixture<ControlPad>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [ControlPad]
    })
    .compileComponents();

    fixture = TestBed.createComponent(ControlPad);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
