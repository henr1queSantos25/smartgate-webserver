#include <stdio.h>     
#include <string.h>              
#include <stdlib.h>              

#include "pico/stdlib.h"        
#include "pico/cyw43_arch.h"     

#include "lwip/pbuf.h"           
#include "lwip/tcp.h"           
#include "lwip/netif.h"

#include "lib/hcSR04.h"
#include "lib/ledRGB.h"
#include "lib/buzzer.h"
#include "lib/ssd1306.h"
#include "lib/led_5x5.h"
#include "lib/font.h"

//======================================================
// DEFINIÇÕES E CONFIGURAÇÕES GLOBAIS
//======================================================

// Definições de pinos
#define TRIGGER 16 // Pino trigger do sensor ultrassônico
#define ECHO 17 // Pino echo do sensor ultrassônico
#define I2C_PORT i2c1 // Porta I2C para display OLED
#define I2C_SDA 14 // Pino SDA da interface I2C
#define I2C_SCL 15 // Pino SCL da interface I2C
#define SSD1306_ADDRESS 0x3C // Endereço I2C do display OLED

// Configurações Wi-Fi
#define WIFI_SSID "SEU_SSID"
#define WIFI_PASSWORD "SUA_SENHA"

// Definição da Máquina de Estados
typedef enum {
    ESPERANDO, // Estado inicial - portão fechado, sem presença
    PRESENCA_DETECTADA, // Presença detectada, portão ainda fechado
    PORTAO_ABERTO // Portão aberto para acesso
} EstadoSistema;

// Variáveis globais
EstadoSistema estadoAtual = ESPERANDO; // Estado inicial do sistema
ssd1306_t ssd; // Estrutura do display OLED
uint64_t distancia = 150; // Distância medida pelo sensor (cm)
volatile bool tocar_som_abertura = false; // Controla o som de da "Porta/Portão" que abriu
volatile bool tocar_som_fechamento = false; // Controla o som de da "Porta/Portão" que fechou

//======================================================
// PROTÓTIPOS DE FUNÇÕES
//======================================================
void setup();
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void user_request(const char *request);

//======================================================
// FUNÇÃO PRINCIPAL
//======================================================
int main() {
    setup(); // Inicialização de hardware e configurações

    // Inicialização e configuração do Wi-Fi
    while (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    // Conexão à rede Wi-Fi
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Exibe o endereço IP atribuído
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configuração do servidor TCP na porta 80
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

     // Som de inicialização do sistema
    somInicializacao(BUZZER2);
    while (true) {
        cyw43_arch_poll();  // Processa eventos do Wi-Fi
        distancia = getCmFiltered(TRIGGER, ECHO, 6); // Mede a distância com filtragem para reduzir ruídos
 
        // Limpa o display para nova renderização
        ssd1306_fill(&ssd, false);

        // Toca o som da abertura da Porta/Portão quando necessário
        if (tocar_som_abertura) {
            tocar_som_abertura = false;
            buzzer_pwm_off(BUZZER2);
            somAberturaPortao(BUZZER2); 
        }

        // Toca o som de fechamento da Porta/Portão quando necessário
        if (tocar_som_fechamento) {
            tocar_som_fechamento = false;
            somFechamentoPortao(BUZZER2);
        }

        // Máquina de estados do sistema
        switch (estadoAtual) {
            case ESPERANDO:
                // Transição para PRESENCA_DETECTADA se detectar objeto próximo
                if (distancia <= 30) {
                    estadoAtual = PRESENCA_DETECTADA;
                }
                setLeds(0, 0, 1); // LED Azul indica modo de espera
                drawImage(&ssd, cadeado_fechado); // Mostra ícone de cadeado fechado
                apagarMatriz(); // Apaga a matriz LED
                break;

            case PRESENCA_DETECTADA:
                // Retorna para ESPERANDO se não houver mais presença
                if (distancia > 30) {
                    estadoAtual = ESPERANDO;
                }
                desenhoX(); // Desenha um "X" na matriz LED
                setLeds(1, 0, 0); // LED Vermelho indica alerta
                drawImage(&ssd, alerta); // Mostra ícone de alerta
                alarmePresencaPWM(BUZZER1); // Aciona o alarme sonoro
                apagarMatriz(); // Apaga a matriz LED
                break;

            case PORTAO_ABERTO:
                desenhoCheck(); // Desenha um "check" na matriz LED
                setLeds(0, 1, 0);  // LED Verde indica portão aberto
                drawImage(&ssd, cadeado_aberto);  // Mostra ícone de cadeado aberto
                break;
        }

        ssd1306_send_data(&ssd); // Atualiza o display com as alterações
        sleep_ms(50); // Pequeno atraso para estabilidade
    }

    cyw43_arch_deinit();
    return 0;
}

//======================================================
// FUNÇÕES DE INICIALIZAÇÃO
//======================================================
void setup() {
    stdio_init_all(); // Inicializa stdio
    setupUltrasonicPins(TRIGGER, ECHO); // Configura pinos do sensor ultrassônico
    setupLED(LED_RED); // Configura LED vermelho
    setupLED(LED_GREEN); // Configura LED verde
    setupLED(LED_BLUE); // Configura LED azul
    setup_I2C(I2C_PORT, I2C_SDA, I2C_SCL, 400 * 1000); // Configura I2C a 400kHz
    setup_ssd1306(&ssd, SSD1306_ADDRESS, I2C_PORT); // Inicializa display OLED
    setup_PIO(); // Configura matriz LED 5x5
    init_pwm_buzzer(BUZZER1); // Inicializa buzzer 1 com PWM
    init_pwm_buzzer(BUZZER2); // Inicializa buzzer 2 com PWM
}


//======================================================
// FUNÇÕES DE REDE E SERVIDOR WEB
//======================================================

// Função chamada quando uma nova conexão TCP é aceita
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Processa solicitações do usuário recebidas via HTTP
void user_request(const char *request) {  // Recebe um ponteiro simples (const para segurança)
    if (strstr(request, "GET /abrir_portao") != NULL) {
        if (estadoAtual != PORTAO_ABERTO) {
            tocar_som_abertura = true;  // Seta flag
            estadoAtual = PORTAO_ABERTO;
        }
    } 
    else if (strstr(request, "GET /fechar_portao") != NULL) {
        if (estadoAtual != ESPERANDO) {
            tocar_som_fechamento = true;  // Seta flag
            estadoAtual = (distancia > 30) ? ESPERANDO : PRESENCA_DETECTADA;
        }
    }
}

// Função chamada quando dados são recebidos em uma conexão TCP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // Verifica se a conexão está sendo fechada
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Buffer estático para armazenar a requisição
    char request[1024];  // Tamanho fixo suficiente para requisições HTTP simples
    int len = p->len < 1023 ? p->len : 1023;  // Limita ao tamanho do buffer
    memcpy(request, p->payload, len);
    request[len] = '\0';  // Garante terminação nula

    // Processa a requisição HTTP
    user_request(request);

    // Prepara a string de status para a página web
    char status[64];
    if (estadoAtual == ESPERANDO)
        strcpy(status, "Portao fechado – sem presença detectada");
    else if (estadoAtual == PRESENCA_DETECTADA)
        strcpy(status, "Presença detectada – aguardando ação");
    else
        strcpy(status, "Portao aberto – acesso autorizado");

    // Cria o corpo da página HTML
    static char body[750];
    int body_len = snprintf(body, sizeof(body),
        "<!DOCTYPE html>\n"
        "<meta http-equiv=\"refresh\" content=\"3\">"
        "<html lang='pt-br'>\n"
        "<head><meta charset='UTF-8'><title>Portão Inteligente</title></head>\n"
        "<style>\n"
        "body { background-color: #e3f6fc; font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
        "h1 { font-size: 60px; margin-bottom: 28px;}\n"
        "button { background-color: LightGray; font-size: 32px; margin: 10px; padding: 18px 38px; border-radius: 10px; }\n"
        "</style>\n"
        "<body>\n"
        "<h1>Portão Inteligente</h1>\n"
        "<p>Status: <strong>%s</strong></p>\n"
        "<p>Distância: <strong>%.1f cm</strong></p>\n"
        "<form action='/abrir_portao'><button>Abrir Portão</button></form>\n"
        "<form action='/fechar_portao'><button>Fechar Portão</button></form>\n"
        "</body>\n"
        "</html>\n",
        status, (float)distancia);

    // Cria o cabeçalho HTTP
    char header[128];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n", body_len);

    
    // Envia o cabeçalho e o corpo da resposta HTTP
    tcp_write(tpcb, header, header_len, TCP_WRITE_FLAG_COPY);
    tcp_write(tpcb, body, body_len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    // Libera o buffer de recebimento
    pbuf_free(p);
    return ERR_OK;
}

