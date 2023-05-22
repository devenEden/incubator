#include <avr/io.h>
#include <util/delay.h>

#define LCD_DDR DDRD
#define LCD_PORT PORTD
#define RS PD2
#define EN PD3
#define D4 PD4
#define D5 PD5
#define D6 PD6
#define D7 PD7

void lcd_init(void);
void lcd_command(unsigned char cmnd);
void lcd_data(unsigned char data);
void lcd_print(char* str, uint8_t row);
void lcd_write_at(char* str, uint8_t row, uint8_t col);
void adc_init(void);
uint16_t read_adc(uint8_t channel);
char* int_to_str(int value);
int read_up_button(void);
int read_set_button(void);
int read_down_button(void);

char buffer[16] = "Temp: ";

int main(void) {
  adc_init(); // initialize the ADC
  LCD_DDR = 0xFF;
  lcd_init(); // initialize the LCD
  lcd_command(0x01); // clear the display
  lcd_print("    WELCOME!    ", 0);
  lcd_print("   INCUBATOR   ", 1);
  _delay_ms(3000);
  lcd_command(0x01); // clear the display
  float voltage;
  float temperature;
  int middle_button = 1;
  int up_button = 0;
  int down_button = 2;
  int set_temperature_screen = 0; 
  int previous_set_button_state = 0;
  int count = 24;
  Serial.begin(9600);  
  DDRB = 0b111000; 
  PORTB = 0b000111;
  
  while (1) {
    // read button state
    int up_button_state = read_up_button();
    int set_button_state = read_set_button();
    int down_button_state = read_down_button();
    
    uint16_t adc_val1 = read_adc(0); // read from analog input 0
    voltage = 0.004882 * adc_val1;  
    temperature = (voltage - 0.5) * 100;
    char* temp_str = int_to_str(temperature);
    char buffer[16] = "C    ";
    strcat(temp_str,buffer);
    
    if (set_button_state == 1 && previous_set_button_state == 0) {
      // enter set mode
      set_temperature_screen = 1;
      previous_set_button_state = 1;
      
      // check if up button
    }
    else if (set_button_state == 1 && previous_set_button_state == 1) {
      // leave set mode 
      set_temperature_screen = 0;
      previous_set_button_state = 0;
    }
    
    if (set_temperature_screen == 0) {
      lcd_write_at("TEMP: ", 0, 0);
      lcd_write_at(temp_str, 0, 6);
    }
    else {
       if (up_button_state == 1) {
        count = count + 1;
      	}
      	else if (down_button_state == 1) {
        count = count - 1;
     	}
      	else {
        count = count;
      	}
      lcd_write_at("SET TEMP: ", 0, 0);
      lcd_write_at(int_to_str(count), 0, 10);
    }
    
    
    if (temperature > count) {
      // turn on the motor
      PORTB |= 0b010000;
    }
    else if (temperature < count) {
      // turn on the heater
      PORTB |= 0b100000;
    } 
    
    free(temp_str);
  }
}

void lcd_init(void) {
  _delay_ms(15);
  lcd_command(0x02); // return home
  lcd_command(0x28); // 4-bit mode, 2 lines, 5x7 format
  lcd_command(0x0C); // display on, cursor off, blink off
  lcd_command(0x06); // entry mode, auto increment with no shift
}

void lcd_command(unsigned char cmnd) {
  LCD_PORT = (LCD_PORT & 0x0F) | (cmnd & 0xF0); // send high nibble
  LCD_PORT &= ~(1 << RS); // set RS pin to 0 for command mode
  LCD_PORT |= (1 << EN); // toggle EN pin to enable LCD
  _delay_us(1);
  LCD_PORT &= ~(1 << EN);
  _delay_us(200);
  LCD_PORT = (LCD_PORT & 0x0F) | (cmnd << 4); // send low nibble
  LCD_PORT |= (1 << EN); // toggle EN pin to enable LCD
  _delay_us(1);
  LCD_PORT &= ~(1 << EN);
  _delay_ms(2);
}

void lcd_data(unsigned char data) {
  LCD_PORT = (LCD_PORT & 0x0F) | (data & 0xF0); // send high nibble
  LCD_PORT |= (1 << RS); // set RS pin to 1 for data mode
  LCD_PORT |= (1 << EN); // toggle EN pin to enable LCD
  _delay_us(1);
  LCD_PORT &= ~(1 << EN);
  _delay_us(200);
  LCD_PORT = (LCD_PORT & 0x0F) | (data << 4); // send low nibble
  LCD_PORT |= (1 << EN); // toggle EN pin to enable LCD
  _delay_us(1);
  LCD_PORT &= ~(1 << EN);
  _delay_ms(2);
}

void lcd_print(char* str, uint8_t row) {
  if (row == 0) {
    lcd_command(0x80); // set cursor to the beginning of the first line
  } else if (row == 1) {
    lcd_command(0xC0); // set cursor to the beginning of the second line
  } else {
    return; // invalid row number
  }
  for (uint8_t i = 0; i < strlen(str); i++) {
    lcd_data(str[i]);
  }
}

void lcd_write_at(char* str, uint8_t row, uint8_t col) {
  uint8_t addr = (row == 0) ? col : 0x40 + col; // compute DDRAM address based on row and column
  lcd_command(0x80 | addr); // set cursor to DDRAM address
  for (uint8_t i = 0; str[i] != '\0'; i++) {
    lcd_data(str[i]); // write each character to the LCD
  }
}

void adc_init(void) {
  // set the reference voltage to AVCC with external capacitor at AREF pin
  ADMUX = (1 << REFS0);

  // enable ADC and set prescaler to 128 (ADC clock = 16 MHz / 128 = 125 kHz)
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t read_adc(uint8_t channel) {
  // set the ADC channel to read from
  ADMUX &= 0xF0;
  ADMUX |= channel;

  // start the conversion and wait for it to complete
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  // read the ADC result
  uint16_t adc_val = ADCL;
  adc_val |= (ADCH << 8);

  return adc_val;
}

char* int_to_str(int value) {
  char str[12]; // create a temporary buffer to hold the result
  itoa(value, str, 10); // convert the integer to a string with base 10
  char* result = (char*)malloc(strlen(str) + 1); // dynamically allocate memory for the result
  strcpy(result, str); // copy the string to the result buffer
  return result; // return a pointer to the result buffer
}


int read_up_button() {
  // Initialize variables
   //enable the pull up resistor 
  int button = 0;

  // Read the value from the pin and debounce the input
  if (PINB & 0b000001) {
      // PORTB &= ~ 0b001000;
    }
    else {
    //  PORTB |= 0b001000;
      button  = 1;
     _delay_ms(500);
    }
  // Return the button value
  return button;
}

int read_set_button() {
  // Initialize variables
   //enable the pull up resistor 
  int button = 0;

  // Read the value from the pin and debounce the input
  if (PINB & 0b000010) {
     // PORTB &= ~ 0b001000;
    }
    else {
      //PORTB |= 0b001000;
      button  = 1;
     _delay_ms(500);
    }
  // Return the button value
  return button;
}

int read_down_button() {
  // Initialize variables
   //enable the pull up resistor 
  int button = 0;

  // Read the value from the pin and debounce the input
  if (PINB & 0b000100) {
    //  PORTB &= ~ 0b001000;
  }
   else {
    //  PORTB |= 0b001000;
      button  = 1;
     _delay_ms(500);
   }
  // Return the button value
  return button;
}


