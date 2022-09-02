#include <SPI.h>                              // библиотека для работы с шиной SPI
#include "nRF24L01.h"                         // библиотека радиомодуля
#include "RF24.h"                             // ещё библиотека радиомодуля
#include <GyverPower.h>                       // для сна

RF24 radio(7, 8);                             // "создать" модуль на пинах 

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб
uint32_t myTimer1;
byte counter = 1;
byte myData;                                  // относительно этого будет строиться дальнейшая логика

#define buttonPin 0                           // номер входа, подключенный к кнопке получается пин D2

/*пользовательские настройки*/
#define n 0                                   // номер трубы на которой работает передатчик (значения от 0 до 5)
#define waiting_time 10000                    // время ожидания ответа приемника


void setup() {
  Serial.begin(9600);                         //открываем порт для связи с ПК
  // все для сна
  power.autoCalibrate();
  power.setSleepMode(POWERDOWN_SLEEP);
  attachInterrupt(buttonPin, wakeup, RISING); // RISING, если хотим ловить 1 на пине. Иначе FALLING

  radio.begin();                              //активировать модуль
  radio.setAutoAck(1);                        //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);                    //(время между попыткой достучаться, число попыток)
  //radio.enableAckPayload();                 //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(1);                    //размер пакета, в байтах

  //radio.openReadingPipe(1,address[1]);      //хотим слушать трубу 1
  radio.openWritingPipe(address[n]);          //мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x70);                     //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX);             //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS);           //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  //radio.powerUp();                          //начать работу
  radio.stopListening();                      //не слушаем радиоэфир, мы передатчик
}

void loop() {
  power.sleep(SLEEP_FOREVER);                 // заснуть
  radio.powerUp();                            // хочу веселья
  delay(10);
  Serial.print("Sent: "); Serial.println(counter);
  radio.write(&counter, sizeof(counter));     // отправляем наши данные 
  radio.startListening();                     // хотим слушать
  radio.openReadingPipe (n, address[n]);      // открываем ту же трубу для прослушки
  myTimer1 = millis();                        // для обновления таймера
  while(millis() - myTimer1 <= waiting_time){ // ждем ответа
    if(radio.available()){                    // проверяем не пришел ли ответ
      radio.read( &myData, sizeof(myData) );
      Serial.print("Получил: ");
      Serial.println(myData);                 // получил
      radio.closeReadingPipe(n);              // закрываем все нахуй 
      break;
    }  
  }
  radio.stopListening();                      // не хочу ничего слушать
  radio.openWritingPipe(address[n]);          // буду говорить по трубе
  radio.powerDown();                          // но сначала спать
  //delay(1000);                              // используем для тестирования так как прерывания слишком быстрые
}
                                              // бесполезная функция XD
void wakeup(){
  }
