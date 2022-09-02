#include <SPI.h>
#include <nRF24L01.h>                                          // Подключаем файл настроек из библиотеки RF24.
#include <RF24.h>                                              // Подключаем библиотеку для работы с nRF24L01+.
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <iarduino_RTC.h>                                      // библиотека для часов реального вуремени   
#include <SoftwareSerial.h>                                    // Подключаем библиотеку SoftwareSerial
#include <DFPlayerMini_Fast.h>                                 // Подключаем библиотеку DFPlayerMini_Fast
#include "GyverEncoder.h"                                      // библиотека для энкодера     
#include "images.h"                                            // файл с нашими изображениями                               

/*все для nrf*/
RF24     radio(6, 7);                                          // Создаём объект radio для работы с библиотекой RF24, указывая номера выводов модуля (CE, SS).
// имена передатчиков
char* transmiter[6] = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
byte myData;                                                   // Объявляем массив для приёма и хранения данных (до 32 байт включительно).
uint8_t  r_pipe;
uint32_t  pipe [6];                                            // Объявляем переменную в которую будет сохраняться номер трубы по которой приняты данные.
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб
#define waiting_time 10000                                     // время ожидания ответа на передатчик
#define button 5                                               // подключаем кнопку ответа

/*настройка матрицы*/
#define pinMatrix 10                                           // подключаем матрицу на сs -> 10
#define numberOfHorizontalDisplays 1                           // количество матриц по-горизонтали
#define numberOfVerticalDisplays 2                             // количество матриц по-вертикали
#define brightness 0                                           // яркость матрицы от 0 до 15
#define wait 100                                               // время между крайними перемещениями букв milliseconds
#define spacer 1                                               // Промежуток между символами (кол-во точек)
uint32_t myTimer;
String tape = "";                                              // строка с руссифицированым текстом
const uint8_t width = 5 + spacer;                              // размер шрифта 
bool flag = HIGH;                                              // для состояния точек на матрицах и подчеркивания
// иницилизируем матрицу
Max72xxPanel matrix = Max72xxPanel(pinMatrix, numberOfHorizontalDisplays, numberOfVerticalDisplays);

/*пины для энкодера*/
#define CLK 8
#define DT 9
#define SW 4

/*подключаем mp3*/
#define pin_TX 7                                                // artuinio TX mp3 player RX
#define pin_RX 8                                                // artuinio RX mp3 player TX
SoftwareSerial MP3_Serial(pin_RX, pin_TX);                      // Указываем к какими портам подключен DFPlayer (RX, TX)
DFPlayerMini_Fast MP3;                                          // Создаем объект 

Encoder enc1(CLK, DT, SW);                                      // Объявляем объект enc для работы с функциями и методами библиотеки                                               
iarduino_RTC clocks(RTC_DS1307);                                // указываем наш модуль часов

/*функция для вывода изображения сигнала, тут можно и подключить mp3 модуль*/
void display_call(char n){
  MP3.play(1);                                                  // Воспроизведение трека 0001
  for(uint8_t i = 0; i < 9; i++){
      matrix.fillScreen(LOW);
      matrix.drawChar(0, 1, n, HIGH, LOW, 1);
      for(uint8_t y = 0; y < 8; y++)
        for(uint8_t x = 0; x < 8; x++){
            matrix.drawPixel(8 + x, y, signals[i][y] & (1<<x));
        }
      matrix.write();
      while(millis()%200){
        question(); 
      }
    }
}

/*вывод текста на матрицу*/
String display_print(String str){
  String tape = utf8rus(str);
  for ( int i = 0 ; i < width * tape.length() + matrix.width() - 1 - spacer; i++ ){
    matrix.fillScreen(LOW);

    int letter = i / width; // номер символа выводимого на матрицу 
    
    int x = (matrix.width() - 1) - i % width;  
    int y = (matrix.height() - 8) / 2; // center the text vertically

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < tape.length() ) {
        matrix.drawChar(x, y, tape[letter], HIGH, LOW,1);
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    while(millis() % wait){
      question();
      //ловим нажатие
      if(button_status())
        return "1";
    }
    //delay(wait);
  }
  return "0";
}

/* Recode russian fonts from UTF-8 to Windows-1251 */
String utf8rus(String source){
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x2F;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB7; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x6F;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
  return target;
}

/*убираем дребезг кнопки*/
bool button_status(){
  if(digitalRead(button)){
    delay(50);
    if(digitalRead(button))
      return 1; 
  }
  return 0;
}

/*вывод передатчика на матрицы*/
void display_transmitter(uint8_t n){
  char c = n + '0';
  while(true){
    // выводим домики
    for(uint8_t i = 0; i < 4; i++){
      matrix.fillScreen(LOW);
      matrix.drawChar(0, 1,c, HIGH, LOW, 1);
      for(uint8_t y = 0; y < 8; y++)
        for(uint8_t x = 0; x < 8; x++){
            matrix.drawPixel(8 + x, y, images[i][y] & (1<<x));
        }
      matrix.write();
      while(millis()%500){
        question(); 
        if(pipe[n] > 0){
          if(button_status()){ 
            display_call(c);
            sending(n, 1);
            update_time();
            return;
          }
        }
        else{
          update_time();
          return;
        }
      }
    }
  // выводим наименования 
    if(display_print(transmiter[n]) == "1"){ 
      display_call(c);
      sending(n, 1);
      update_time();
      return;
    }
  }   
  update_time();
}

/*вывод нижнего подчеркивание*/
void display_underscores(uint8_t k){
  for(uint8_t x = k; x < k + 8; x++)
    matrix.drawPixel(x, 7, flag);
  flag = !flag;
  matrix.write();
}

/*функция для вывода точек*/
void display_points(){
  uint8_t n = 1;
  while(n < 5){
    for(uint8_t y = n; y < n + 2; y++){
      for(uint8_t x = 7; x < 9; x++){
          matrix.drawPixel(x, y, flag);
        }
    }
    n += 3;
  }
  flag = !flag;
  matrix.write();
}

/*функция вывода выремени на матрицу*/
void display_time(uint8_t hour, uint8_t minute){
  uint8_t timer[4];                                                  // массив для разбития чисел на символы
  timer[0] = hour / 10;
  timer[1] = hour % 10;
  timer[2] = minute / 10;
  timer[3] = minute % 10;
  for(uint8_t y = 0; y < 7; y++){
    uint8_t n = 0, i = 0;
    while(n < 16){
      for(uint8_t x = 0; x < 4; x++){
        // зажигаем x-й пиксель в y-й строке
        matrix.drawPixel(n + x, y, nambers[timer[i]][y] & (1<<x));
      }
      i++;
      n += 4;
      if(i == 2)
        n++;
    }
  }
  matrix.write();
}

/*обновление времени*/
void update_time(){
  clocks.gettime("H:i");
  matrix.fillScreen(LOW);
  display_time(clocks.Hours, clocks.minutes);
}

/*функция для установки вермени*/
void setup_time() {
  uint8_t m, h;                                                  // переменные для получения текущего времени
  h = clocks.Hours;
  m = clocks.minutes;
  matrix.fillScreen(LOW);                                            // очищаем матрицу
  display_time(h, m);
  do{
     question();
     if(millis()%500==0){                                                     
      delay(1);
      display_underscores(0);
     }                                      
     if (enc1.isRight() and h < 23){ 
      ++h;
      display_time(h, m);
     }
     if (enc1.isLeft() and h > 0){
      --h;
      display_time(h, m);
     }
  }while(!enc1.isPress());                                       //пока кнопка не нажата 
  matrix.fillScreen(LOW);                                            // очищаем матрицу
  display_time(h, m);
  do{
     question();
     if(millis()%500==0){                                                         
      delay(1);
      display_underscores(8);
     }
     if (enc1.isRight() and m < 59){
      ++m;
      display_time(h, m);
     }
     if (enc1.isLeft() and m > 0){
      --m;
      display_time(h, m);
     }
  }while(!enc1.isPress());
  clocks.settime(0, m, h, -1, -1, -1, -1);                         // сек, мин, час, день, октября, года, день недели
  matrix.fillScreen(LOW);                                            // очищаем матрицу
  display_time(h, m);
} 

/*функция для проверки времени хранения передатчиков в ожидание*/
void mini_stack(){
  for(int i = 0; i < 6; i++)
      if(millis() - pipe[i] >= waiting_time and pipe[i] > 0)
        sending(i, 0);
}

/*отправка ответа на передатчик*/
void sending(uint8_t trub, byte data_s){
  radio.closeReadingPipe(trub);
  radio.openWritingPipe(address[trub]);
  radio.stopListening();
  radio.write(&data_s, sizeof(data_s));
  radio.openReadingPipe (trub, address[trub]);
  radio.startListening();
  pipe[trub] = 0;
  if(data_s){
    Serial.print("Пользователь ответил на передатчик №");
    Serial.println(trub);
  }else
    Serial.print("Время истекло на передатчик №");
    Serial.println(trub);
}

/*проверка наличия новых передатчиков*/
void question(){ 
  if(radio.available(&r_pipe)){
    radio.read( &myData, sizeof(myData) );
    if(!pipe[r_pipe])
      pipe[r_pipe] = millis();
    Serial.print("Добавлен передатчик №");
    Serial.println(r_pipe);
    display_transmitter(r_pipe);
  }
  enc1.tick();                                                                    // обязательная функция отработки. Должна постоянно опрашиваться
  mini_stack();
}

void setup() {
  Serial.begin(9600);
  Serial.println("START");

  enc1.setType(TYPE2);                                   // тип энкодера TYPE1 одношаговый, TYPE2 двухшаговый. Если ваш энкодер работает странно, смените тип
  clocks.begin();

  radio.begin();                                         // Инициируем работу nRF24L01+.
  radio.setAutoAck(1);                                   //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setChannel      (0x70);                          // Указываем канал передачи данных (от 0 до 125), 27 - значит приём данных осуществляется на частоте 2,427 ГГц.
  radio.setDataRate     (RF24_250KBPS);                  // Указываем скорость передачи данных (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS), RF24_1MBPS - 1Мбит/сек.
  radio.setPALevel      (RF24_PA_MAX);                   // Указываем мощность передатчика (RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBm, RF24_PA_MAX=0dBm).
  radio.openReadingPipe (0, address[0]);                 // Открываем 1 трубу с адресом 1 передатчика 0xAABBCCDD11, для приема данных.
  radio.openReadingPipe (1, address[1]);                 // Открываем 1 трубу с адресом 1 передатчика 0xAABBCCDD11, для приема данных.
  radio.openReadingPipe (2, address[2]);                 // Открываем 2 трубу с адресом 2 передатчика 0xAABBCCDD22, для приема данных.
  radio.openReadingPipe (3, address[3]);                 // Открываем 3 трубу с адресом 3 передатчика 0xAABBCCDD33, для приема данных.
  radio.openReadingPipe (4, address[4]);                 // Открываем 4 трубу с адресом 4 передатчика 0xAABBCCDD96, для приема данных.
  radio.openReadingPipe (5, address[5]);                 // Открываем 5 трубу с адресом 5 передатчика 0xAABBCCDDFF, для приема данных.
  radio.startListening  ();                              // Включаем приемник, начинаем прослушивать открытые трубы.
  radio.setPayloadSize(1);                               //размер пакета, в байтах
  radio.setAutoAck(1);                                   //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);                               //(время между попыткой достучаться, число попыток)
  radio.powerUp();                                       //начать работу

  pinMode(button, INPUT);
  
  MP3_Serial.begin(9600);                                // Открываем последовательную связь
  MP3.begin(MP3_Serial);                                 // инициализация
  MP3.volume(20);                                        // Указываем громкость (0-30)
  
  /*вывод теста на матрицу*/
  matrix.setIntensity(brightness);                       // яркость матрицы от 0 до 15
  matrix.setRotation(matrix.getRotation()+3);            // 1 - 90  2 - 180   3 - 270
  display_print("ИНВАКРЫМ.рф");                          // воспроизведениt текста
  update_time();
}

void loop() {
  question();                                            // должна постоянно опрашиваться
  if(millis()%1000==0){                                  // мигаем точками раз в 1 сек
    delay(1);
    display_points();
  }
  if(millis() - myTimer >= 60000){                       // если прошла 1 мин
    myTimer = millis();                                  // сброс таймера
    update_time();                                       // обновляем время 
  }

  if(enc1.isPress())
    setup_time();
}
