# Projeto Final - Internet das Coisas (IoT) 2025.2

Esse repositório consiste no projeto final realizado pelos colaborados para a disciplina de Internet das Coisas na graduação de Engenharia da Computação na Universidade Federal de São Paulo.

O projeto consiste no código para um alimentador de gatos utilizando esp32 e arduinos.

# Componentes
- Sensor LDR para medir a quantidade de luz no reservatório
- Sensor Ultrassônico para medir a quantidade de ração dentro do pote
- Sensor de Cor pra ver se tem ou não ração na vasilha(vasilha é cônica.)
- Sensor DHT para medir a umidade da vasilha pra ver se libera a ração
- Servo motor para abrir e fechar a vasilha
- Envio de informações por CoAP

## Informações no dashboard
- HTML, CSS
- Se precisa ou não trocar a ração
- Quantas vezes o gato se alimentou no dia
- A umidade da vasilha


## LEDs
- Primeiro: verifica se todos os componentes estão OK para liberar a comida. Você aperta um botão que atualiza essa verificação e, se tiver tudo ok, ele apaga o LED.

