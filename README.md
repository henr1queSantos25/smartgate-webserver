# SmartGate: Controle Inteligente de Acesso via Wi-Fi

O **SmartGate** é um sistema embarcado de controle de acesso inteligente desenvolvido para a plataforma **BitDogLab** com o microcontrolador **Raspberry Pi Pico W**. O sistema detecta aproximação de pessoas ou veículos usando um sensor ultrassônico HC-SR04 e permite autorização remota para abertura de portões através de uma interface Web acessível via rede Wi-Fi. O projeto integra múltiplos periféricos para fornecer feedback visual e sonoro sobre o estado atual do sistema, criando uma solução completa de controle de acesso.

---

## Funcionalidades Principais

- **Detecção de Presença em Tempo Real**: Utiliza sensor ultrassônico HC-SR04 para detectar aproximação de pessoas ou veículos com distância programável (≤ 30 cm).
- **Controle Remoto via Wi-Fi**: Interface Web embarcada no microcontrolador, acessível de qualquer dispositivo na mesma rede Wi-Fi.
- **Sistema de Notificação Multi-sensorial**:
  - Indicações visuais através de LED RGB e matriz de LEDs 5x5.
  - Alertas sonoros distintos para diferentes situações via buzzer com PWM.
  - Informações detalhadas e iconografia no display OLED.
- **Operação baseada em Máquina de Estados**:
  - **Modo Esperando**: Estado padrão quando não há presença detectada.
  - **Presença Detectada**: Acionado quando alguém se aproxima do portão.
  - **Portão Aberto**: Estado ativado após autorização remota via interface Web.
- **Atualizações Automáticas**: A interface Web atualiza automaticamente a cada 3 segundos para exibir o status atual.

---

## Tecnologias Utilizadas

- **Linguagem de Programação**: C  
- **Microcontrolador**: Raspberry Pi Pico W (RP2040 + CYW43439)  
- **Conectividade**: Wi-Fi embutido (modo Station)
- **Servidor Web**: Implementado com lwIP TCP/IP stack
- **Componentes Utilizados**:
  - Sensor ultrassônico HC-SR04 para detecção de presença
  - Display OLED SSD1306 128x64 (I2C) para informações visuais
  - LED RGB para indicação de estado
  - Matriz de LEDs 5x5 para alertas visuais dinâmicos
  - Buzzers com PWM para alertas sonoros
- **Bibliotecas**:
  - lwIP para comunicação de rede
  - Pico SDK para acesso ao hardware (GPIO, ADC, PWM e I2C)
  - CYW43 para controle do módulo Wi-Fi
  - Bibliotecas personalizadas para controle dos periféricos

---

## Como Funciona

### Detecção de Presença
- O sensor ultrassônico HC-SR04 mede continuamente a distância entre o portão e qualquer objeto à sua frente.
- Leituras são filtradas para reduzir ruídos e garantir precisão (múltiplas amostragens).
- Quando a distância medida é ≤ 30 cm, o sistema considera que há uma presença detectada.

### Máquina de Estados
- **ESPERANDO**:
  - LED RGB: Azul
  - Display OLED: Ícone de cadeado fechado
  - Matriz LED: Desligada
  - Buzzer: Desligado
  - Transição: Passa para PRESENCA_DETECTADA se distância ≤ 30 cm

- **PRESENCA_DETECTADA**:
  - LED RGB: Vermelho
  - Display OLED: Ícone de alerta
  - Matriz LED: Símbolo "X" piscante
  - Buzzer: Alarme sonoro ativo
  - Transição: Retorna para ESPERANDO se distância > 30 cm ou avança para PORTAO_ABERTO se autorizado via Web

- **PORTAO_ABERTO**:
  - LED RGB: Verde
  - Display OLED: Ícone de cadeado aberto
  - Matriz LED: Símbolo "✓" (check)
  - Buzzer: Som de abertura (momentâneo)
  - Transição: Retorna para estado apropriado (ESPERANDO ou PRESENCA_DETECTADA) quando fechado via Web

### Interface Web
- Página HTML simples e responsiva servida diretamente pelo microcontrolador.
- Exibe status atual do sistema e distância medida em tempo real.
- Botões para "Abrir Portão" e "Fechar Portão".
- Atualização automática a cada 3 segundos.
- Acessível via endereço IP do dispositivo em um navegador web.

---

## Configuração do Hardware

| Componente | Pino do RP2040 | Função |
|------------|----------------|--------|
| Sensor HC-SR04 Trigger | GP16 | Dispara pulso ultrassônico |
| Sensor HC-SR04 Echo | GP17 | Recebe resposta do pulso ultrassônico |
| Display OLED (I2C) | GP14 (SDA), GP15 (SCL) | Exibição de informações e ícones |
| LED RGB | Pinos dedicados (LED_RED, LED_GREEN, LED_BLUE) | Indicação visual de estado |
| Matriz de LEDs 5x5 | PIO | Exibição de símbolos de alerta e confirmação |
| Buzzer 1 | BUZZER1 (PWM) | Alarme para presença detectada |
| Buzzer 2 | BUZZER2 (PWM) | Sons de inicialização, abertura e fechamento |
| Wi-Fi (CYW43439) | Integrado ao Pico W | Comunicação sem fio e servidor web |

---

## Estrutura do Repositório

- **`smartgate_webserver.c`**: Código-fonte principal do projeto.
- **`CMakeLists.txt`**: Arquivo de configuração para o sistema de build CMake.
- **`lib/hcSR04.h` e `lib/hcSR04.c`**: Biblioteca para o sensor ultrassônico HC-SR04.
- **`lib/ssd1306.h` e `lib/ssd1306.c`**: Biblioteca para controle do display OLED.
- **`lib/led_5x5.h` e `lib/led_5x5.c`**: Biblioteca para controle da matriz de LEDs 5x5 via PIO.
- **`lib/buzzer.h` e `lib/buzzer.c`**: Biblioteca para geração de sons via PWM.
- **`lib/ledRGB.h` e `lib/ledRGB.c`**: Biblioteca para controle do LED RGB.
- **`lib/font.h`**: Definição da fonte e ícones utilizados no display OLED.
- **`README.md`**: Documentação do projeto.

---

## Fluxo de Operação

1. **Inicialização**: O sistema configura todos os periféricos, inicia o módulo Wi-Fi e estabelece conexão com a rede especificada.
2. **Servidor Web**: Um servidor TCP é iniciado na porta 80 para servir a interface de controle.
3. **Loop Principal**: O sistema continuamente:
   - Lê e filtra a distância medida pelo sensor ultrassônico
   - Atualiza o estado da máquina de estados com base nas medições
   - Gerencia os periféricos (LED RGB, OLED, matriz LED, buzzers) de acordo com o estado atual
   - Processa requisições recebidas via interface Web
   - Reproduz sons específicos em resposta a eventos (abertura/fechamento)

---

## Conceitos Aplicados

- **Máquina de Estados**: Gerenciamento de diferentes modos de operação do sistema.
- **Servidor Web Embarcado**: Interface de controle remoto acessível via navegador.
- **Medição Ultrassônica**: Detecção de presença sem contato físico.
- **Filtragem de Sinais**: Técnicas para redução de ruído nas leituras do sensor.
- **TCP/IP e Wi-Fi**: Comunicação sem fio e protocolo de rede.
- **Modulação por Largura de Pulso (PWM)**: Geração de diferentes padrões sonoros.
- **Interface I2C**: Comunicação com o display OLED.
- **Programable IO (PIO)**: Controle eficiente da matriz de LEDs 5x5.
- **Feedback Multi-sensorial**: Combinação de estímulos visuais e auditivos para alertas.

---

## Objetivos Alcançados

- **Automação Residencial**: Sistema prático para controle de acesso em residências.
- **Interface Remota**: Controle via dispositivos móveis sem necessidade de aplicativos adicionais.
- **Segurança**: Alerta em tempo real para presença detectada.
- **Usabilidade**: Combinação de indicadores visuais e sonoros para feedback completo.
- **Modularidade**: Código organizado em bibliotecas reutilizáveis.
- **Prototipação Realista**: Simulação funcional de um sistema automatizado de controle de portão.
- **Conectividade**: Demonstração prática de projeto IoT com servidor web embarcado.

---

## Desenvolvido por

Henrique Oliveira dos Santos  
[LinkedIn](https://www.linkedin.com/in/dev-henriqueo-santos/)