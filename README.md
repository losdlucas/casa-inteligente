# Projeto IoT ESP32

## Componentes utilizados

Para a montagem do projeto serão utilizados os seguintes componentes:

* ESP32
* Protoboard
* LEDs
* Resistores
* Buzzer
* Jumpers
* Cabo USB
* Computador

Também será utilizada a plataforma Wokwi para fazer a simulação e testar as ligações do circuito antes da montagem física.

### Função dos componentes

O ESP32 é o principal componente do projeto, sendo responsável pelo processamento e pela comunicação com a rede Wi-Fi.

Os LEDs representam as luzes dos cômodos da casa e são utilizados para simular o acionamento das lâmpadas.

Os resistores são utilizados para limitar a corrente dos LEDs e evitar danos aos componentes.

O buzzer será utilizado para emitir sinais sonoros em situações de alerta.

A protoboard facilita a montagem do circuito e os jumpers são utilizados para fazer as conexões entre os componentes.

O cabo USB é utilizado para alimentar e programar o ESP32.

O computador será utilizado para desenvolver o sistema, realizar os testes e executar a aplicação web.

A plataforma Wokwi será utilizada para realizar a prototipagem e verificar se as ligações do circuito estão funcionando corretamente antes da montagem física.

## Funcionamento do projeto

O projeto tem como objetivo criar um sistema de casa inteligente utilizando ESP32. O dispositivo poderá receber comandos enviados pela aplicação web e controlar os componentes do circuito.

A comunicação entre o sistema e o ESP32 será realizada utilizando MQTT. Além disso, os eventos realizados no sistema serão armazenados em um banco de dados PostgreSQL utilizando o Neon.

O projeto também contará com uma aplicação Flask, que será responsável por receber os dados enviados pelo sistema, processar as informações e realizar a comunicação com o banco de dados e com o MQTT.

## Prototipagem

A prototipagem do circuito será realizada no Wokwi. Essa etapa permite testar o funcionamento dos componentes e verificar as conexões antes de realizar a montagem física.

O projeto possui o arquivo `diagram.json`, que contém a representação do circuito utilizado na simulação.
