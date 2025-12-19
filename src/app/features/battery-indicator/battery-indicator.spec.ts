import { ComponentFixture, TestBed } from '@angular/core/testing';

import { BatteryIndicator } from './battery-indicator';

describe('BatteryIndicator', () => {
  let component: BatteryIndicator;
  let fixture: ComponentFixture<BatteryIndicator>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [BatteryIndicator]
    })
    .compileComponents();

    fixture = TestBed.createComponent(BatteryIndicator);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
