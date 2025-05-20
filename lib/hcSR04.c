#include "hcSR04.h"

// Tempo máximo de espera pelo retorno do pulso (em microssegundos)
int timeout = 26100;


// Configura os pinos do sensor ultrassônico
void setupUltrasonicPins(uint trigPin, uint echoPin) {
    gpio_init(trigPin);
    gpio_init(echoPin);
    gpio_set_dir(trigPin, GPIO_OUT);
    gpio_set_dir(echoPin, GPIO_IN);
}


// Obtém a duração do pulso de eco em microssegundos
uint64_t getPulse(uint trigPin, uint echoPin) {
    // Envia pulso de trigger de 10μs
    gpio_put(trigPin, 1);
    sleep_us(10);
    gpio_put(trigPin, 0);

    uint64_t width = 0;

    // Aguarda até que o pino echo fique HIGH (com timeout)
    uint64_t wait_count = 0;
    while (gpio_get(echoPin) == 0) {
        tight_loop_contents();
        wait_count++;
        sleep_us(1);
        if (wait_count > timeout) return 0; // Timeout esperando ECHO HIGH
    }
    absolute_time_t startTime = get_absolute_time();
    // Mede o tempo até que o pino echo volte para LOW
    while (gpio_get(echoPin) == 1) 
    {
        width++;
        sleep_us(1);
        // Retorna 0 se exceder o tempo máximo de espera
        if (width > timeout) return 0;
    }
    absolute_time_t endTime = get_absolute_time();
    
    return absolute_time_diff_us(startTime, endTime);
}

// Obtém a distância em centímetros
uint64_t getCm(uint trigPin, uint echoPin) {
    uint64_t pulseLength = getPulse(trigPin, echoPin);
    return pulseLength / 29 / 2;  // Fórmula: (tempo em μs) / 29 / 2 = distância em cm
}


// Obtém a distância em polegadas
uint64_t getInch(uint trigPin, uint echoPin) {
    uint64_t pulseLength = getPulse(trigPin, echoPin);
    return (long)pulseLength / 74.f / 2.f;  // Fórmula: (tempo em μs) / 74 / 2 = distância em polegadas
}

// Obtém a distância filtrada em centímetros usando múltiplas amostras
uint64_t getCmFiltered(uint trigPin, uint echoPin, int samples) {
    // Coleta várias amostras
    uint64_t values[samples];
    for (int i = 0; i < samples; i++)
    {
        values[i] = getCm(trigPin, echoPin);
        sleep_ms(10);  // Pequeno atraso entre leituras
    }

    // Ordenar os valores para descartar extremos
    for (int i = 0; i < samples - 1; i++)
    {
        for (int j = i + 1; j < samples; j++)
        {
            if (values[i] > values[j])
            {
                uint64_t temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }

    // Retornar a mediana (ou média sem extremos)
    return values[samples / 2];
}