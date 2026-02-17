import { ComponentFixture, TestBed } from '@angular/core/testing';

import { RssiRadar } from './rssi-radar';

describe('RssiRadar', () => {
  let component: RssiRadar;
  let fixture: ComponentFixture<RssiRadar>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [RssiRadar]
    })
    .compileComponents();

    fixture = TestBed.createComponent(RssiRadar);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
