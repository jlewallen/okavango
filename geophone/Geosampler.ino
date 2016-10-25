/****************************************************************************
 * geosampler.ino
 *
 * Record geophone data and output the data (from 0 through 4095) to the
 * serial port.
 *
 * See the file COPYRIGHT for copyright details.
 ***************************************************************************/
// Arduino: 1.6.10 (Windows 10), Board: "Arduino/Genuino Mega or Mega 2560, ATmega2560 (Mega 2560)"

// C:\Source\page5of4\arduino-1.6.10\arduino-builder -dump-prefs -logger=machine -hardware "C:\Source\page5of4\arduino-1.6.10\hardware" -hardware "C:\Users\jlewa\AppData\Local\Arduino15\packages" -tools "C:\Source\page5of4\arduino-1.6.10\tools-builder" -tools "C:\Source\page5of4\arduino-1.6.10\hardware\tools\avr" -tools "C:\Users\jlewa\AppData\Local\Arduino15\packages" -built-in-libraries "C:\Source\page5of4\arduino-1.6.10\libraries" -libraries "C:\Users\jlewa\OneDrive\Documents\Arduino\libraries" -fqbn=arduino:avr:mega:cpu=atmega2560 -ide-version=10610 -build-path "C:\Users\jlewa\AppData\Local\Temp\build8b6c85f44d62e5b0de8d523ea9ca94dc.tmp" -warnings=none -prefs=build.warn_data_percentage=75 -verbose "C:\Source\okavango\geophone\geophone.ino"

#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

/* Serial speed for the report generation.  It should be fast enough to
   allow several values to be passed per second.  A speed of 38,400 baud
   should suffice for worst case reports of about 2,600 bytes per second. */
#define SERIAL_SPEED    115200

/* The geophone data is sampled on analog pin 5. */
#define GEODATA_PINX          3
#define GEODATA_PINY          4
#define GEODATA_PINZ          5

/* Make an LED blink on every successful report. */
#define REPORT_BLINK_ENABLED   1
#define REPORT_BLINK_LED_PIN  13

/* Define the geophone data sampling rate. */
#define SAMPLE_RATE   512

/* Define the on-board LED so we can turn it off. */
#define LED_PIN             13

#define NUMBER_OF_GEODATA_SAMPLES 256

void error(char *str);

typedef struct geodata_t {
    uint8_t pin;
    /* Create a double buffer for geodata samples. */
    short geodata_samples[ NUMBER_OF_GEODATA_SAMPLES * 2 ];
    short *geodata_samples_real;
    /* Indexes used by the interrupt service routine. */
    int  isr_current_geodata_index;
    /* Semaphor indicating that a frame of geophone samples is ready. */
    bool geodata_buffer_full;
} geodata_t;

geodata_t geophones[3];

/* Flag that indicates that a report with amplitude information was
   created.  It is used by the report LED blinking. */
bool report_was_created;

/**
 * Setup the timer interrupt and prepare the geodata sample buffers for
 * periodic sampling.  Timer1 is used to generate interrupts at a rate of
 * 512 Hz.
 *
 * This function is board specific; if other board than the Arduino Mega
 * or the Arduino Due are used the code must be updated.
 */
void start_sampling( )
{
  geophones[0].pin = GEODATA_PINX;
  geophones[1].pin = GEODATA_PINY;
  geophones[2].pin = GEODATA_PINZ;

  /* Prepare the buffer for sampling. */
  geophones[0].isr_current_geodata_index = 0;
  geophones[0].geodata_buffer_full       = false;
  geophones[1].isr_current_geodata_index = 0;
  geophones[1].geodata_buffer_full       = false;
  geophones[2].isr_current_geodata_index = 0;
  geophones[2].geodata_buffer_full       = false;

  /* Setup interrupts for the Arduino Mega. */
#if defined( ARDUINO_AVR_MEGA2560 ) || defined( ARDUINO_AVR_UNO ) || defined( ARDUINO_AVR_DUEMILANOVE )
  // Set timer1 interrupt to sample at 512 Hz. */
  const unsigned short prescaling     = 1;
  const unsigned short match_register = F_CPU / ( prescaling * SAMPLE_RATE ) - 1;
  cli( );
  TCCR1B = ( TCCR1B & ~_BV(WGM13) ) | _BV(WGM12);
  TCCR1A = TCCR1A & ~( _BV(WGM11) | _BV(WGM10) );
  TCCR1B = ( TCCR1B & ~( _BV(CS12) | _BV(CS11) ) ) | _BV(CS10);
  OCR1A = match_register;
  TIMSK1 |= _BV(OCIE1A);
  sei( );

  /* Setup interrupts the Arduino Due. */
#elif defined( ARDUINO_SAM_DUE )
  /* Set a 12-bit resolutiong. */
  analogReadResolution( 12 );
  /* Disable write protect of PMC registers. */
  pmc_set_writeprotect( false );
  /* Enable the peripheral clock. */
  pmc_enable_periph_clk( TC3_IRQn );
  /* Configure the channel. */
  TC_Configure( TC1, 0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4 );
  uint32_t rc = VARIANT_MCK/128/SAMPLE_RATE;
  /* Setup the timer. */
  TC_SetRA( TC1, 0, rc/2 );
  TC_SetRC( TC1, 0, rc );
  TC_Start( TC1, 0 );
  TC1->TC_CHANNEL[ 0 ].TC_IER = TC_IER_CPCS;
  TC1->TC_CHANNEL[ 0 ].TC_IDR = ~TC_IER_CPCS;
  NVIC_EnableIRQ( TC3_IRQn );
#else
#error Arduino board not supported by this software.
#endif
}



#if defined( ARDUINO_AVR_MEGA2560 ) || defined( ARDUINO_AVR_UNO ) || defined( ARDUINO_AVR_DUEMILANOVE )
void sampling_interrupt();

/**
 * Interrupt service routine for Arduino Mega devices which invokes the
 * generic interrupt service routine.
 */
ISR(TIMER1_COMPA_vect)
{
  sampling_interrupt( );
}


#elif defined( ARDUINO_SAM_DUE )
/**
 * Interrupt service routine for Arduino Due devices which invokes the
 * generic interrupt service routine.
 */
void TC3_Handler( )
{
  TC_GetStatus( TC1, 0 );
  sampling_interrupt( );
}


#else
#error Arduino board not supported by this software.
#endif


void report_blink( bool enabled );

/*
 * Interrupt service routine for sampling the geodata.  The geodata analog
 * pin is sampled at each invokation of the ISR.  If the buffer is full, a
 * pointer is passed to the main program and a semaphor is raised to indicate
 * that a new frame of samples is available, and future samples are written
 * to the other buffer.
 *
 * While not a sampling task, we take advantage of the timer interrupt to
 * blink the report LED if enabled.
 */
void sampling_interrupt( )
{
  /* Read a sample and store it in the geodata buffer. */
#if defined( ARDUINO_AVR_MEGA2560 ) || defined( ARDUINO_AVR_UNO ) || defined( ARDUINO_AVR_DUEMILANOVE )
  const int adc_resolution = 1024;
#elif defined( ARDUINO_SAM_DUE )
  const int adc_resolution = 4096;
#endif
  for (uint8_t i = 0; i < 3; ++i) {
      geodata_t *gd = &geophones[i];

      short geodata_sample = analogRead( gd->pin >> 1 );
      /* Scale the sample. */
      const int scale = 8192 / adc_resolution;
      geodata_sample = (short)( (double)geodata_sample * scale );
      gd->geodata_samples[ gd->isr_current_geodata_index++ ] = geodata_sample;

      /* Raise a semaphor if the buffer is full and tell which buffer
         is active. */
      if( gd->isr_current_geodata_index == NUMBER_OF_GEODATA_SAMPLES )
      {
          gd->geodata_samples_real     = &gd->geodata_samples[ 0 ];
          gd->geodata_buffer_full      = true;
      }
      else if( gd->isr_current_geodata_index == NUMBER_OF_GEODATA_SAMPLES * 2 )
      {
          gd->geodata_samples_real      = &gd->geodata_samples[ NUMBER_OF_GEODATA_SAMPLES ];
          gd->isr_current_geodata_index = 0;
          gd->geodata_buffer_full       = true;
      }
  }

  /* In the same interrupt routine, handle report LED blinking. */
  report_blink( REPORT_BLINK_ENABLED );
}



/**
 * Blink the report LED if it has been enabled.
 *
 * @param enabled @a true if report blinking has been enabled.
 */
void report_blink( bool enabled )
{
  static unsigned long timestamp;
  static bool          led_on = false;

  if( enabled == true )
  {
    /* Turn on the LED and start a timer if a report was created. */
    if( report_was_created == true )
    {
      report_was_created = false;
      timestamp = millis( ) + 50;
      digitalWrite( REPORT_BLINK_LED_PIN, HIGH );
      led_on = true;
    }
    /* Turn off the LED once the timer expires. */
    if( led_on == true )
    {
      if( millis( ) > timestamp )
      {
        digitalWrite( REPORT_BLINK_LED_PIN, LOW );
        led_on = false;
      }
    }
  }
}

RTC_DS1307 RTC;
DateTime now;
File logfile;
String fileName = "";
uint8_t fileNameLength = 10;

void makeNewFileName() {
  now = RTC.now();
  fileName = String(now.unixtime() - 1467360476, DEC) + ".csv";
  fileNameLength = fileName.length() + 1;
  Serial.println(now.unixtime());
  Serial.println(fileName);
}

uint32_t numberOfFlushes = 0;

void makeNewFile() {
  makeNewFileName();
  char a[fileNameLength];
  fileName.toCharArray(a, fileNameLength);
  logfile = SD.open(a, FILE_WRITE);
  if (!logfile) {
    error("couldnt create file");
  }
  numberOfFlushes = 0;
}

void flushLog() {
  logfile.flush();
}

/**
 * Send the samples in the most recent buffer over the serial port.
 *
 * @param [in] freq_real Array of samples.
 * @param [in] length Number of samples.
 */
void report( const short *samples, int length )
{
  /* Send all the samples in the buffer to the serial port. */
  for( int index = 0; index < length; index++ )
  {
    Serial.print( " " );
	Serial.print( samples[ index ] );
  }

  /* Indicate to the report LED blinking that the report was submitted. */
  report_was_created = true;
}

void error(char *str) {
  Serial.print("error: ");
  Serial.println(str);
  while (1);
}

/**
 * Initialize the serial port, setup the sampling, and turn off the on-board
 * LED.
 */
void setup()
{
  /* Initialize the serial port with the desired speed. */
  Serial.begin( SERIAL_SPEED );

  pinMode(10, OUTPUT);
  if (!SD.begin(10, 11, 12, 13)) {
      error("Card failed, or not present");
  }

  Wire.begin();
  if (!RTC.begin()) {
      error("RTC failed");
  }
  if (!RTC.isrunning()) {
      Serial.println("RTC was not running, you will have to run DS1307SetTime.");
      Serial.println("For now, I'll set the RTC to the date and time the program was compiled.");
      Serial.println("For better initialization, run Dave's DS1307SetTime sketch.");
      Serial.println();
      RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  uint8_t isrunning(void);

  /* Setup the geophone data sampling buffers and sampling interrupt. */
  start_sampling( );

  /* Turn off the on-board LED. */
  pinMode( LED_PIN, OUTPUT );
  digitalWrite( LED_PIN, LOW );

  /* Configure the report LED if enabled. */
  report_was_created = false;
  if( REPORT_BLINK_ENABLED )
  {
    pinMode( REPORT_BLINK_LED_PIN, OUTPUT );
    digitalWrite( REPORT_BLINK_LED_PIN, LOW );
  }

  makeNewFile();
}

/**
 * Main program loop which reports the samples every time the sample buffer
 * has been filled.
 */
void loop()
{
  uint16_t lastMinute = 0;
  while (true) {
    /* Analyze the geophone data once it's available. */
    bool allFull = true;
    for (uint8_t i = 0; i < 3; ++i) {
      geodata_t *gd = &geophones[i];
      if( !gd->geodata_buffer_full )
      {
        allFull = false;
      }
    }
    if (allFull) {
      now = RTC.now();

      for (uint32_t i = 0; i < NUMBER_OF_GEODATA_SAMPLES; ++i) {
        for (uint8_t i = 0; i < 3; ++i) {
          geodata_t *gd = &geophones[i];
          logfile.print(gd->geodata_samples[i]);
          logfile.print(",");
          logfile.print(gd->geodata_samples[i]);
          logfile.print(",");
          logfile.println(gd->geodata_samples[i]);

          gd->geodata_buffer_full = false;
        }
      }

      flushLog();

      int16_t minute = now.minute();
      if (minute % 3 == 0 && lastMinute != minute) {
          flushLog();
          makeNewFile();
          lastMinute = minute;
      }
    }
  }
}
