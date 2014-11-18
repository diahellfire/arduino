#include "DHT.h"
#include <EEPROM.h>

#define DHTTYPE DHT22 // тип DHT -> 22
#define DHT_UP_PIN 4 // Пин верхнего температурного датчика
#define DHT_DOWN_PIN 5 // Пин нижнего температурного датчика
#define PUMP_PIN 6 // Пин управления реле компрессора
#define MENU_PIN 9 // Buttons menu
#define TEMP_UP_PIN 10 // Button up
#define TEMP_DOWN_PIN 11 // button down
#define SET_PIN 12 // Button set
#define OPERATE_PIN 13 // Button set
#define BEEP_PIN 7 // Button set

int select_camera = 0; // default 0 down camera (1 up camera)
int new_up_temp = 0; // default new temperature
int new_down_temp = 0; // default new temperature
boolean menu = false;
boolean pump = false;
boolean serial = false;
int target_temp_up;
int target_temp_down;
int up_trigger;
int down_trigger;

DHT dht_up(DHT_UP_PIN, DHTTYPE); // Создаем объект датчик 1
DHT dht_down(DHT_DOWN_PIN, DHTTYPE); // Создаем объект датчик 2

int EET_UP; // переменная температур для верхней камеры
int EET_DOWN; // переменная температур для нижней камеры
int EET_UP_TRIGGER; //триггер отрицательной температуры верхней камеры
int EET_DOWN_TRIGGER; //триггер отрицательной температуры нижней камеры

void setup(void) {
  pinMode(BEEP_PIN, OUTPUT); // beeper

  pinMode(MENU_PIN, INPUT); // Кнопка (menu)
  pinMode(TEMP_UP_PIN, INPUT); // Кнопка (up)
  pinMode(TEMP_DOWN_PIN, INPUT); // Кнопка (down)
  pinMode(SET_PIN, INPUT); // Кнопка (set)

  //pinMode(MENU_PIN, INPUT_PULLUP);
  //pinMode(TEMP_UP_PIN, INPUT_PULLUP);
  //pinMode(TEMP_DOWN_PIN, INPUT_PULLUP);
  //pinMode(SET_PIN, INPUT_PULLUP);
  digitalWrite(MENU_PIN, HIGH); //resistor pull-up
  digitalWrite(TEMP_UP_PIN, HIGH); //resistor pull-up
  digitalWrite(TEMP_DOWN_PIN, HIGH); //resistor pull-up 
  digitalWrite(SET_PIN, HIGH); //resistor pull-up 

  pinMode(PUMP_PIN, OUTPUT); // определяем пин компрессора
  pinMode(OPERATE_PIN, OUTPUT); // определяем пин для статуса мигалки

  if(serial == true){
    Serial.begin(115200); //com port
  }
  dht_up.begin(); // DHT init ...
  dht_down.begin(); // DHT init ...
  
  digitalWrite(PUMP_PIN, HIGH); // предварительно отключаем компрессор
  if(serial == true){
    Serial.println("Refrigerator started...");
  }
}

void loop(void) {

  EET_UP = EEPROM.read(1); // address[1] температура из eeprom (верхняя камера)
  EET_DOWN = EEPROM.read(2); // address[2] температура eeprom (морозилка)
  EET_UP_TRIGGER = EEPROM.read(3); // Триггер отрицательной температуры (верхняя камера) [1 - (minus)] [0 +]
  EET_DOWN_TRIGGER = EEPROM.read(4); // Триггер отрицательной температуры (морозилка) [1 - (minus)] [0 +]

  float ha = dht_up.readHumidity(); // Считываем влажность датчика 1
  float hb = dht_down.readHumidity(); // Считываем влажность датчика 2
  float ta = dht_up.readTemperature(); // Считываем температуру датчика 1
  float tb = dht_down.readTemperature(); // Считываем температуру датчика 2
if(serial == true){
  Serial.println("==========================================================================");
}
if(serial == true){
  if (isnan(ha) || isnan(hb)) {
    Serial.println("Failed to read from DHT Humidity");
  } 
  if (isnan(ta) || isnan(tb)) {
    Serial.println("Failed to read from DHT Temperature");
  } 
}
  // set all to int
  int curr_up_temp = (int) ta; // конвертим float to int
  int curr_down_temp = (int) tb; // конвертим float to int
  int up_delta_on = 2; // Пределы расхождений верхней камеры для включения (ON)
  int up_delta_off = 1; // Пределы расхождений верхней камеры для отключения (OFF)
  int down_delta_on = 5; // Пределы расхождений морозилки для включения (ON)
  int down_delta_off = 2; // Пределы расхождений морозилки для отключения (OFF)
  int corr_temp_up = neg(EET_UP, EET_UP_TRIGGER); // определяем отрицательные температуры из триггеров (верхняя камера)
  int corr_temp_down = neg(EET_DOWN, EET_DOWN_TRIGGER); // определяем отрицательные температуры из триггеров (морозилка)
  int float_temp_down_on = corr_temp_down + down_delta_on; // плавающее значение включения (-10) + delta-on (5) = (-5) = включение
  int float_temp_down_off = corr_temp_down - down_delta_off; // плавающее значение выключения (-10) - delta-off (2) = (-12) = выключение

if(serial == true){
  //Serial.print("EE Current state Address Up: ");
  //Serial.println(EET_UP);
  //Serial.print("EE Current state Address Down: ");
  //Serial.println(EET_DOWN);
  //Serial.print("EE Up temperature is negative: ");
  //Serial.println(EET_UP_TRIGGER);
  //Serial.print("EE Down temperature is negative: ");
  //Serial.println(EET_DOWN_TRIGGER);
  Serial.print("Target temperature [up camera]: ");
  Serial.print(corr_temp_up);  
  Serial.print(" [down camera]: ");
  Serial.println(corr_temp_down);
  Serial.print("Current temperature [up camera]: ");
  Serial.print(curr_up_temp);
  Serial.print(" [down camera]: ");
  Serial.println(curr_down_temp);
  Serial.print("Compressor state: ");
  Serial.println(pump);
  Serial.print("Float temperature ON: ");
  Serial.print(float_temp_down_off);
  Serial.print(" OFF: ");
  Serial.println(float_temp_down_on);
}
  if(curr_down_temp >= float_temp_down_on && pump == false){ // если текущая температура выше или равна плавающей температуре включения и компрессор выключен
    if(serial == true){
      Serial.println(">>>>>>>>>>> Compressor is ON <<<<<<<<<<<<");
    }
    digitalWrite(PUMP_PIN, LOW); // включаем компрессор
    pump = true; //запоминаем, что компрессор включился
  }
  if(curr_down_temp <= float_temp_down_off && pump == true){ // если текущая температура меньше или равна плавающей температуре отключения и компрессор включен
    if(serial == true){
      Serial.println(">>>>>>>>>>> Compressor is OFF <<<<<<<<<<<<");
    }
    digitalWrite(PUMP_PIN, HIGH); // льключаем компрессор
    pump = false; // запоминаем что компрессор выключен
  }

  //unfrezzzzzz режим разморозки.

  mainmenu(); // меню установок температур
  blinkLED(OPERATE_PIN); // помигиваем лампочкой что процессор работает
}

int mainmenu(){
  if(menu == true){
    if(serial == true){
    Serial.print("Menu >>> selected [0 - down camera | 1 - up camera]: ");
    Serial.println(select_camera);
    Serial.print("New temperature >>> Up camera: ");
    Serial.print(new_up_temp);
    Serial.print(" Down camera: ");
    Serial.println(new_down_temp);
    }
  }
  int MENU_PIN_val = digitalRead(MENU_PIN);
  int TEMP_UP_PIN_val = digitalRead(TEMP_UP_PIN);
  int TEMP_DOWN_PIN_val = digitalRead(TEMP_DOWN_PIN);
  int SET_PIN_val = digitalRead(SET_PIN);
  
  //Serial.println(MENU_PIN_val, DEC);
  //Serial.println(TEMP_UP_PIN_val, DEC);
  //Serial.println(TEMP_DOWN_PIN_val, DEC);
  //Serial.println(SET_PIN_val, DEC);
     
  if (MENU_PIN_val == 0){ // нажатие кнопки меню
    if(serial == true){
      Serial.println("Enter menu");
    }    
    menu = true; // запоминаем вход в меню
    beep(100);
  }
  if (MENU_PIN_val == 0 && menu == true){ // нажитие на кнопку меню
     if(select_camera == 1){
	select_camera = 0; // select down camera
     }else{
        select_camera = 1; // select up camera
     }
  beep(100);
  if(serial == true){
    Serial.print("Setup temperature [0 - down camera | 1 - up camera]:");
    Serial.println(select_camera);
  }  
  }
  // обработка установок для верхней камеры
  if (TEMP_UP_PIN_val == 0 && menu == true && select_camera == 1){ // нажали кнопку +
    beep(50);
    new_up_temp++; // новая температура
    if(serial == true){
      Serial.print("Temperature [up camera] (+): ");
      Serial.println(new_up_temp);
    }
  }
  if (TEMP_DOWN_PIN_val == 0 && menu == true && select_camera == 1){ // нажали кнопку -
    beep(50);
    new_up_temp--; // новая температура
    if(serial == true){
      Serial.print("Temperature [up camera] (-): ");
      Serial.println(new_up_temp);
    }
  }
  // обработка установок для морозилки
  if (TEMP_UP_PIN_val == 0 && menu == true && select_camera == 0){ // нажали кнопку +
    beep(50);
    new_down_temp++; // новая температура
    if(serial == true){
      Serial.print("Temperature [down camera] (+): ");
      Serial.println(new_down_temp);
    }
  }
  if (TEMP_DOWN_PIN_val == 0 && menu == true && select_camera == 0){ // нажали кнопку -
    beep(50);
    new_down_temp--; // новая температура
    if(serial == true){
      Serial.print("Temperature [down camera] (-): ");
      Serial.println(new_down_temp);
    }
  }

  if (SET_PIN_val == 0 && menu == true){ // нажали кнопку "сет"
    // ==================================================================================
    if (new_up_temp != 0){ // если есть установки новой температуры для верхней камеры
	if(new_up_temp < 0){
		up_trigger = 1; // температура ниже 0
		target_temp_up = 0 - (new_up_temp);
        }else{
		up_trigger = 0; // температура выше 0
		target_temp_up = new_up_temp;
	}
	EEPROM.write(1, target_temp_up);
	EEPROM.write(3, up_trigger);
        new_up_temp = 0; //обнуляем
        if(serial == true){
        Serial.print("Saved [up camera] setting up: ");
        Serial.println(target_temp_up);
        Serial.print("Triggered:");
        Serial.println(up_trigger);
        }
    }
    // ==================================================================================
    if (new_down_temp != 0){ // если есть установки новой температуры для морозилки
	if(new_down_temp < 0){
		down_trigger = 1; // температура ниже 0
		target_temp_down = 0 - (new_down_temp);
        }else{
		down_trigger = 0; // температура выше 0
		target_temp_down = new_down_temp;
	}
	EEPROM.write(2, target_temp_down);
	EEPROM.write(4, down_trigger);
        new_down_temp = 0; //обнуляем
        if(serial == true){
        Serial.print("Saved [down camera] setting up: ");
        Serial.println(target_temp_down);
        Serial.print("Triggered:");
        Serial.println(down_trigger);
        }
    }
    // ==================================================================================
    menu = false; // exit menu
    beep(100);
    if(serial == true){
      Serial.println("Exit menu");
    }
  }
}

// функция преобразования в отрицательное число
int neg(int temp, int trigger){
  int result;
  if(trigger == 1){
	result = 0 - temp;
  } else {
  	result = temp;
  }
  return result;
}

// пищалка
int beep(unsigned char delayms){
  analogWrite(BEEP_PIN, 200);
  delay(delayms); 
  analogWrite(BEEP_PIN, 0);
  delay(delayms);   
}  

// процессинг
int blinkLED(int count)
{
        digitalWrite(13, HIGH);
        delay(10);
        digitalWrite(13, LOW);
        delay(10);
}

