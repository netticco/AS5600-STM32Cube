# Cntrolador de RPM de um motor DC escvado (STM32F411 + AS5600)

Este projeto implementa um tacômetro digital para motores DC utilizando o microcontrolador **STM32F411 (Blackpill)** e o sensor magnético **AS5600**. O sistema é projetado para medir rotações na faixa de **400 a 4000 RPM** com alta resolução temporal.

##  Especificações de Hardware

* **Microcontrolador**: STM32F411CEU6 (Blackpill) operando com HCLK de **60MHz**.
* **Sensor**: AS5600 (Saída Analógica configurada para detecção de borda digital EXTI).
* **Gerador de Sinais (Validação)**: Tektronix AFG1022.
* **Interface de Dados**: USB CDC (Virtual COM Port) nativo.

### Conexões (Pinagem)

| Componente | Pino STM32 | Função |
| :--- | :--- | :--- |
| **Sensor (OUT)** | **PB0** | Entrada de Interrupção (EXTI0) |
| **Driver (EN)** | **PA4** | Habilitação do Driver de Potência |
| **Driver (PWM)** | **PA8/PA9** | Sinais de controle do motor (TIM1) |
| **USB-C** | **PA11/PA12** | Comunicação de dados (DP/DM) |

---

##  Tratamento de Sinal e Debounce

A transição do sensor AS5600 (Rampa) pode gerar ruído elétrico durante a descida brusca da tensão, o que causa gatilhos falsos na interrupção digital do microcontrolador.

* **Solução**: Implementação de um filtro capacitivo direto.
* **Componente**: Adicionado um capacitor de **10nF** em paralelo entre o pino **PB0** e o **GND**.
* **Efeito**: Suavização de picos de alta frequência, garantindo que a interrupção `HAL_GPIO_EXTI_Callback` seja disparada apenas uma vez por revolução completa.

---

##  Lógica de Cálculo e Performance

A velocidade é calculada medindo-se o intervalo de tempo ($\Delta t$) entre duas bordas de descida sucessivas utilizando o **TIM2** (Timer de 32 bits) com resolução de **$1 \mu s$**.

A fórmula matemática aplicada é:

$$RPM = \frac{60 \cdot 10^6}{\Delta t}$$

* **Taxa de Amostragem**: O sistema envia atualizações para o Serial Plotter a cada **50ms** (20Hz).
* **Debounce de Software**: Implementado um gate de tempo de **$1.000 \mu s$**

---

##  Desafios Técnicos e "Spikes" Documentados

Durante transições bruscas de velocidade ou mudanças de frequência no gerador, o sistema apresenta picos de leitura falsos (**spikes**).

* **Análise**: O erro ocorre devido ao cálculo de RPM ser baseado em um único pulso instantâneo. Qualquer variação mínima na fase do sinal ou no tempo de descarga do capacitor de filtro distorce o $\Delta t$ calculado.
* **Status**: O comportamento está documentado para correção em versões futuras através de algoritmos de filtragem digital (como Filtro de Mediana).

---

##  Roadmap

1. [X] Definição dos pinos utlizados.
2. [X] Leitura da saída analógica + Plot via Serial do RPM.
3. [ ] Acionamento do motor via Half Bridge.
4. [ ] Controle de malha fechada utilizando algoritmo **PID**.
3. [ ] Ajuste dinâmico de PWM via potenciômetro (ADC).
