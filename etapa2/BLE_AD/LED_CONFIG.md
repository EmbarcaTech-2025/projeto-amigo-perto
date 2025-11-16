# Configuração do LED Customizado

## Pino Atual
O LED está configurado para o pino **P1.07** (GPIO1, pino 7).

## Como Mudar o Pino do LED

Para alterar o pino do LED, edite o arquivo:
```
boards/nrf52840dongle_nrf52840.overlay
```

### Exemplo: Mudar para P0.13
```dts
leds {
    compatible = "gpio-leds";
    custom_led: led_0 {
        gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;  // P0.13
        label = "Custom LED";
    };
};
```

### Exemplo: Mudar para P1.10
```dts
leds {
    compatible = "gpio-leds";
    custom_led: led_0 {
        gpios = <&gpio1 10 GPIO_ACTIVE_LOW>;  // P1.10
        label = "Custom LED";
    };
};
```

## Formato do Pino
```
gpios = <&gpioX Y FLAGS>;
```

Onde:
- **X** = 0 ou 1 (porta GPIO: gpio0 ou gpio1)
- **Y** = número do pino (0-31)
- **FLAGS** = GPIO_ACTIVE_LOW ou GPIO_ACTIVE_HIGH
  - `GPIO_ACTIVE_LOW`: LED acende quando o pino está em LOW (comum com resistor pull-up)
  - `GPIO_ACTIVE_HIGH`: LED acende quando o pino está em HIGH

## Exemplos de Pinos nRF52840
- P0.06 = `<&gpio0 6 GPIO_ACTIVE_LOW>`
- P0.13 = `<&gpio0 13 GPIO_ACTIVE_LOW>`
- P1.00 = `<&gpio1 0 GPIO_ACTIVE_LOW>`
- P1.07 = `<&gpio1 7 GPIO_ACTIVE_LOW>` (atual)
- P1.10 = `<&gpio1 10 GPIO_ACTIVE_LOW>`

## Rebuild
Após modificar o overlay, faça um rebuild completo do projeto para aplicar as mudanças.
