export function enableBluetoothSimulation() {
  console.log('SIMULAÇÃO DE BLUETOOTH ATIVADA');

  const fakeDevice = {
    name: 'Amigo Perto (Simulado)',
    gatt: {
      connect: async () => {
        console.log('[Simulador] Conectando ao GATT...');
        return Promise.resolve({
          getPrimaryService: async (serviceUuid: string) => {
            console.log(`[Simulador] Obtendo serviço: ${serviceUuid}`);
            return Promise.resolve({
              getCharacteristic: async (characteristicUuid: string) => {
                console.log(
                  `[Simulador] Obtendo característica: ${characteristicUuid}`
                );
                return Promise.resolve({
                  writeValue: async (value: Uint8Array) => {
                    console.log(
                      `[Simulador] Comando recebido: ${value[0]}`
                    );
                    return Promise.resolve();
                  },
                });
              },
            });
          },
          disconnect: () => {
            console.log('[Simulador] Desconectado.');
          },
        });
      },
    },
  };

  // Sobrescreve a função do navegador
  (navigator as any).bluetooth.requestDevice = async (options: any) => {
    console.log('[Simulador] requestDevice chamado com opções:', options);
    return Promise.resolve(fakeDevice);
  };
}
