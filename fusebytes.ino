char Arduino_preprocess_edge;  // hint to Arduino pre-processor

#include <avr/boot.h>
#include <EEPROM.h>
#include "cpuname.h"

#define BOOT_READABLE (1<<BLB12)

#include <avr/pgmspace.h>
#define fp(string) flashprint(PSTR(string))


#if 0
/*
 * These were used to fill up flash to debug some issues with
 * optiboot uploads of large sketches.
 */

#define make64bytes(a)  FLASH_STRING(__flash1 ## a, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")

#define make256bytes(a) make64bytes(x ## a);     make64bytes(x1 ##a);	\
    make64bytes(x2 ## a);    make64bytes(x3 ## a)

#define make1kbytes(a) make256bytes(x1 ## a);     make256bytes(x2 ## a); \
    make256bytes(x3 ## a);     make256bytes(x4 ## a)

#define make4kbytes(a) make1kbytes(x1 ## a);     make1kbytes(x2 ## a); \
    make1kbytes(x3 ## a);     make1kbytes(x4 ## a)

#define make16kbytes(a) make4kbytes(x1 ## a);     make4kbytes(x2 ## a); \
    make4kbytes(x3 ## a);     make4kbytes(x4 ## a)

make16kbytes(foo);
make4kbytes(bar);
make256bytes(baz);
make256bytes(baz1);
make256bytes(baz2);
#endif

/*
 * SIGRD is a "must be zero" bit on most Arduino CPUs;
 * can we read the sig or not?  (Apparently, only on picopower parts)
 */
#if (!defined(SIGRD))
#define SIGRD 5
#endif

unsigned char fuse_bits_low;
uint8_t fuse_bits_extended;
uint8_t fuse_bits_high;
uint8_t lock_bits;
uint8_t sig1, sig2, sig3;
uint8_t oscal;

#define FLASHSIZE (FLASHEND+1)

uint8_t *bootaddr = (uint8_t *) 0;
unsigned short bootlen = 0;

void setup()
{
  Serial.begin(19200);
}

void space() {
  Serial.print(' ');
}

void flashprint (const char p[])
{
    uint8_t c;
    while (0 != (c = pgm_read_byte(p++))) {
	Serial.write(c);
    }
}

void print_serno(void)
{
  int i;
  int unoSerial[6];
  int startAddr=1018;
  unsigned long serno = 0;

  for (i = 0; i < 6; i++) {
    unoSerial[i] = EEPROM.read(startAddr + i);
  }
  if (unoSerial[0] == 'U' && unoSerial[1] == 'N' && unoSerial[2] == 'O') {

      fp("Your Serial Number is: UNO");

    for (i = 3; i < 6; i = i + 1) {
      serno = serno*256 + unoSerial[i];
      Serial.print(" ");
      Serial.print(unoSerial[i], HEX);
    }
    fp(" (");
    Serial.print(serno);
    Serial.write(')');
  } 
  else {
      fp("No Serial Number");
  }
  Serial.println();
}

void print_binary(uint8_t b)
{
  for (uint8_t i=0x80; i>0; i>>=1) {
    if (b&i) {
      Serial.write('1');
    } 
    else {
      Serial.write('0');
    }
  }
}

/*
 * print_fuse_low
 * Explain the contents of the "low" fuse byte.
 * 
 * Note that most fuses are active-low, and the the avr include files
 * define them as inverted bitmasks...
 */

void print_fuse_low(void)
{
    uint8_t b;
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || \
  defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) ||  \
    defined(AVR_ATmega168A__) || defined(__AVR_ATmega168PA__) ||  \
    defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || \
    defined(__AVR_ATmega8__)
    /*
     * Fuse Low is same on 48/88/168/328/1280/2560
     */
    fp("Fuse Low = ");
  print_binary(fuse_bits_low);
  fp(" (");
  Serial.print(fuse_bits_low, HEX);
  fp(")\n");
  fp("           ||||++++______");
#if FLASHSIZE > 16000
  switch (fuse_bits_low & 0xF) {
  case 0: 
      fp("Reserved");
    break;
  case 1: 
      fp("External Clock");
    break;
  case 2: 
      fp("Calibrated 8MHz Internal Clock");
    break;
  case 3: 
      fp("Internal 128kHz Clock");
    break;
  case 4: 
      fp("LF Crystal, 1K CK Startup");
    break;
  case 5: 
      fp("LF Crystal 32K CK Startup");
    break;
  case 6: 
      fp("Full Swing Crystal ");
    break;
  case 7: 
      fp("Full Swing Crystal");
    break;
  case 8:
  case 9:
      fp("Low Power Crystal 0.4 - 0.8MHz");
    break;
  case 10:
  case 11:
      fp("Low Power Crystal 0.9 - 3.0MHz");
    break;
  case 12:
  case 13:
      fp("Low Power Crystal 3 - 8MHz");
    break;
  case 14:
  case 15:
      fp("Low Power Crystal 8 - 16MHz");
    break;
  }

#else
  b = fuse_bits_low & 15;
  if (b == 1) fp("Ext Clk");
  else if (b == 3) fp("Slow Int Clk");
  else {
      fp("xtal Osc Type=");
      Serial.print((fuse_bits_low) & 15, BIN);
  }
#endif
  Serial.println();
  fp("           ||++__________");
      fp("Start Up Time=");
  Serial.print((fuse_bits_low >> 4) & 3, BIN);
#if !defined(__AVR_ATmega8__)
  Serial.println();
  fp("           |+____________");
      fp("Clock Output ");
      if (fuse_bits_low & (~FUSE_CKOUT))
	  fp("Disabled");
      else
	  fp("Enabled");

  Serial.println();
  fp("           +_____________");
  if (fuse_bits_low & (~FUSE_CKDIV8)) {
      fp("(no divide)");
  } 
  else {
      fp("Divide Clock by 8");
  }
#else
  Serial.println();
  fp("           |+____________");
  if (fuse_bits_low & (~FUSE_BODEN))
      fp("BOD Disabled");
  else
      fp("BOD Enabled");

  Serial.println();
  fp("           +_____________");
  if (fuse_bits_low & (~FUSE_BODLEVEL)) {
      fp("BOD at 2.7V");
  } 
  else {
      fp("BOD at 4.0V");
  }
#endif
#endif
  Serial.println();
}

void print_fuse_high()
{
    fp("\nFuse High = ");
  print_binary(fuse_bits_high);
  fp(" (");
  Serial.print(fuse_bits_high, HEX);
  fp(")\n");

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
  fp("            |||||||+______");
  if (fuse_bits_high & bit(FUSE_BOOTRST)) {
      fp("Reset to Start of memory\n");
  } 
  else {
      fp("Reset to Bootstrap\n");
  }
  fp("            |||||++_______");
  switch ((uint8_t)(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)))) {
    case (uint8_t)((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)):
	fp("256 words (512 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-512);
    break;
    case (uint8_t)((~FUSE_BOOTSZ1)):
	fp("512 words (1024 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-1024);
    break;
    case (uint8_t)((~FUSE_BOOTSZ0)):
	fp("1024 words (2048 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-2048);
    break;
  case 0:
      fp("2048 words (4096 bytes)\n");
      bootaddr = (uint8_t *) (FLASHSIZE-4096);
    break;
  default:
    Serial.println(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)), BIN);
  }
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
  defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__)
  fp("            |||||+++______");
  switch ((uint8_t)(fuse_bits_high & 7)) {
  case 7:
      fp("Brownout Disabled\n");
    break;
  case 6:
      fp("Brownout at 1.8V\n");
    break;
  case 5:
      fp("Brownout at 2.7V\n");
    break;
  case 4:
      fp("Brownout at 4.3V\n");
    break;
  default:	
      fp("Brownout Reserved value");
    Serial.println(fuse_bits_high& 7, BIN);
    break;
  }
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  fp("            |||||||+______");
  if (fuse_bits_high & bit(FUSE_BOOTRST)) {
      fp("Reset to Start of memory\n");
  } 
  else {
      fp("Reset to Bootstrap\n");
  }
  fp("            |||||++_______");
  switch ((uint8_t)(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)))) {
    case (uint8_t)((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)):
	fp("512 words (1024 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-1024);
    break;
    case (uint8_t)((~FUSE_BOOTSZ1)):
	fp("1024 words (2048 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-2048);
    break;
    case (uint8_t)((~FUSE_BOOTSZ0)):
	fp("2048 words (4096 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-4096);
    break;
  case 0:
      fp("4096 words (8192 bytes)\n");
      bootaddr = (uint8_t *) (FLASHSIZE-8192);
    break;
  default:
    Serial.println(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)), BIN);
  }
#elif defined(__AVR_ATmega8__)
  fp("            |||||||+______");
  if (fuse_bits_high & bit(FUSE_BOOTRST)) {
      fp("Reset to 0x0\n");
  } 
  else {
      fp("Reset to Boot\n");
  }
  fp("            |||||++_______");
  switch ((uint8_t)(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)))) {
    case (uint8_t)((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)):
	fp("256 bytes\n");
	bootaddr = (uint8_t *) (FLASHSIZE-256);
    break;
    case (uint8_t)((~FUSE_BOOTSZ1)):
	fp("512 bytes\n");
	bootaddr = (uint8_t *) (FLASHSIZE-512);
    break;
    case (uint8_t)((~FUSE_BOOTSZ0)):
	fp("1024 bytes\n");
	bootaddr = (uint8_t *) (FLASHSIZE-1024);
    break;
  case 0:
      fp("2048 bytes\n");
      bootaddr = (uint8_t *) (FLASHSIZE-2048);
    break;
  default:
    Serial.println(fuse_bits_high & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)), BIN);
  }
#endif
  fp("            ||||+_________");
  if (fuse_bits_high & ~(FUSE_EESAVE)) {
      fp("EEPROM Erased on chip erase\n");
  } 
  else {
      fp("EEPROM Preserved on chip erase\n");
  }
#if !defined(__AVR_ATmega8__)
  fp("            |||+__________");
  if (fuse_bits_high & ~(FUSE_WDTON)) {
      fp("Watchdog programmable\n");
  } 
  else {
      fp("Watchdog always on\n");
  }
  fp("            ||+___________");
  if (fuse_bits_high & ~(FUSE_SPIEN)) {
      fp("ISP programming disabled\n");
  } 
  else {
      fp("ISP programming enabled\n");
  }
#if defined(FUSE_JTAGEN)
#warning Testing for JTAGEN
  /*
   * Big chips have JTAG and OCD
   */
  fp("            |+____________");
  if (fuse_bits_high & ~(FUSE_JTAGEN)) {
      fp("JTAG off\n");
  } 
  else {
      fp("JTAG enabled\n");
  }
  fp("            +_____________");
  if (fuse_bits_high & ~(FUSE_OCDEN)) {
      fp("OCD enabled\n");
  } 
  else {
      fp("OCD disabled\n");
  }
#else
  /*
   * Smaller chips have debugwire and a reprogrammable RESET
   */
   #if defined (FUSE_DWEN)
  fp("            |+____________");
  if (fuse_bits_high & ~(FUSE_DWEN)) {
      fp("DebugWire off\n");
  } 
  else {
      fp("DebugWire enabled\n");
  }
  #endif
  fp("            +_____________");
  if (fuse_bits_high & ~(FUSE_RSTDISBL)) {
      fp("RST enabled\n");
  } 
  else {
      fp("RST disabled\n");
  }
#endif

#else
/* ATmega8 */
  fp("            |||+__________");
  if (fuse_bits_high & ~(FUSE_CKOPT)) {
      fp("Low pwr Osc\n");
  }
  else {
      fp("Full swing Osc\n");
  }
  fp("            ||+___________");
  if (fuse_bits_high & ~(FUSE_SPIEN)) {
      fp("ISP disabled\n");
  } 
  else {
      fp("ISP OK\n");
  }
  fp("            |+____________");
  if (fuse_bits_high & ~(FUSE_WDTON)) {
      fp("Watchdog prog\n");
  } 
  else {
      fp("Watchdog always on\n");
  }
  fp("            +_____________");
  if (fuse_bits_high & ~(FUSE_RSTDISBL)) {
      fp("RST enabled\n");
  } 
  else {
      fp("RST disabled\n");
  }
#endif
}

/*
 * Print info about the contents of the "Extended" fuse byte
 *
 */
void print_fuse_extended()
{
#if !defined(__AVR_ATmega8__)
    fp("\nFuse Extended = ");
  print_binary(fuse_bits_extended);
  fp(" (");
  Serial.print(fuse_bits_extended, HEX);
  fp(")\n");
#endif
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || \
    defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  fp("                |||||+++______");
  switch ((uint8_t)(fuse_bits_extended & 7)) {
  case 7:
      fp("Brownout Disabled\n");
    break;
  case 6:
      fp("Brownout at 1.8V\n");
    break;
  case 5:
      fp("Brownout at 2.7V\n");
    break;
  case 4:
      fp("Brownout at 4.3V\n");
    break;
  default:	
      fp("Brownout Reserved value");
    Serial.println(fuse_bits_extended & 7, BIN);
    break;
  }
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
  fp("                |||||||+______");
  if (fuse_bits_extended & bit(FUSE_BOOTRST)) {
      fp("Reset to Start of memory\n");
  } 
  else {
      fp("Reset to Bootstrap\n");
  }
  fp("                |||||++_______");
  switch ((uint8_t)(fuse_bits_extended & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)))) {
    case (uint8_t)((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)):
	fp("128 words (256 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-256);
    break;
    case (uint8_t)((~FUSE_BOOTSZ1)):
	fp("256 words (512 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-512);
    break;
    case (uint8_t)((~FUSE_BOOTSZ0)):
	fp("512 words (1024 bytes)\n");
	bootaddr = (uint8_t *) (FLASHSIZE-1024);
    break;
  case 0:
      fp("1024 words (2048 bytes)\n"); 
      bootaddr = (uint8_t *) (FLASHSIZE-2048);
      break;
  default:
    Serial.println(fuse_bits_extended & ((~FUSE_BOOTSZ1)+(~FUSE_BOOTSZ0)), BIN);
  }
#elif defined(__AVR_ATmega8__)
//  fp("\nFuse Extended not existent on ATmega8 ");
#endif
}

/*
 * Interpret the lock bits.
 * These seem to be the same on all AVRs that have them!
 */
void print_lock_bits()
{
#if  FLASHSIZE > 16000
    fp("\nLock Bits = ");
  print_binary(lock_bits);
  fp(" (");
  Serial.print(lock_bits, HEX);
  fp(")\n");
  fp("            ||||||++______");
  switch (((uint8_t)(lock_bits & (uint8_t)3))) {
  case 3:
      fp("Read/Write to everywhere\n");
    break;
  case 2:
      fp("Programming of Flash/EEPROM disabled\n");
    break;
  case 0:
      fp("No Read/Write of Flash/EEPROM\n");
    break;
  default:
    Serial.println();
  }
  fp("            ||||++________");
  switch ((uint8_t)((uint8_t)lock_bits & (uint8_t)0b1100)) {  //BLB0x
  case 0b1100:
      fp("R/W Application\n");
    break;
  case 0b1000:
      fp("No Write to App\n");
    break;
  case 0b0000:
      fp("No Write to App, no read from Boot\n");
    break;
  case 0b0100:
      fp("No Write to App, no read from Boot, no Ints to App\n");
    break;
  }

  fp("            ||++__________");
  switch ((uint8_t)((uint8_t)lock_bits & (uint8_t)0b110000)) {  //BLB0x
  case 0b110000:
      fp("R/W Boot Section\n");
    break;
  case 0b100000:
      fp("No Write to Boot Section\n");
    break;
  case 0b000000:
      fp("No Write to Boot, no read from App\n");
    break;
  case 0b010000:
      fp("No Write to Boot, no read from App, no Ints to Boot\n");
    break;
  }
#else
  fp("\nLock Bits = ");
  print_binary(lock_bits);
  fp(" (");
  Serial.print(lock_bits, HEX);
  fp(")\n");
  fp("            ||||||++______");
  switch (((uint8_t)(lock_bits & (uint8_t)3))) {
  case 3:
      fp("ISP RW\n");
    break;
  case 2:
      fp("ISP RO\n");
    break;
  case 0:
      fp("ISP no access\n");
    break;
  default:
    Serial.println();
  }
  fp("            ||||++________");
  switch ((uint8_t)((uint8_t)lock_bits & (uint8_t)0b1100)) {  //BLB0x
  case 0b1100:
      fp("App R/W\n");
    break;
  case 0b1000:
      fp("App RO\n");
    break;
  case 0b0000:
      fp("App No Access\n");
    break;
  case 0b0100:
      fp("App No Access, Ints disabled\n");
    break;
  }

  fp("            ||++__________");
  switch ((uint8_t)((uint8_t)lock_bits & (uint8_t)0b110000)) {  //BLB0x
  case 0b110000:
      fp("Boot R/W\n");
    break;
  case 0b100000:
      fp("Boot RO\n");
    break;
  case 0b000000:
      fp("Boot No Access\n");
    break;
  case 0b010000:
      fp("Boot No Access. Ints disabled\n");
    break;
  }
#endif
}

void print_signature()
{
    fp("\nSignature:         ");
  Serial.print(sig1, HEX);
  space();
  Serial.print(sig2, HEX);
  space();
  Serial.print(sig3, HEX);
  if (sig1 == 0x1E) { /* Atmel ? */
    switch (sig2) {
    case 0x92:  /* 4K flash */
      if (sig3 == 0x0A) 
	  fp(" (ATmega48P)");
      else if (sig3 == 0x05)
	  fp(" (ATmega48A)");
      else if (sig3 == 0x09)
	  fp(" (ATmega48)");
      break;
    case 0x93:  /* 8K flash */
      if (sig3 == 0x0F) 
	  fp(" (ATmega88P)");
      else if (sig3 == 0x0A)
	  fp(" (ATmega88A)");
      else if (sig3 == 0x11)
	  fp(" (ATmega88)");
      else if (sig3 == 0x07)
	  fp(" (ATmega8)");
      break;
    case 0x94:  /* 16K flash */
      if (sig3 == 0x0B) 
	  fp(" (ATmega168P)");
      else if (sig3 == 0x06)
	  fp(" (ATmega168A)");
      break;
    case 0x95:  /* 32K flash */
      if (sig3 == 0x0F)
	  fp(" (ATmega328P)");
      else if (sig3 == 0x14)
	  fp(" (ATmega328)");
      break;
    case 0x96:  /* 64K flash */
      if (sig3 == 0x08)
	  fp(" (ATmega640)");
      else
	  fp(" (????)");
      break;
    case 0x97:  /* 128K flash */
      if (sig3 == 0x03)
	  fp(" (ATmega1280)");
      else if (sig3 == 0x04)
	  fp(" (ATmega1281)");
      break;
    case 0x98:  /* 256K flash */
      if (sig3 == 0x01)
	  fp(" (ATmega2560)");
      else if (sig3 == 0x02)
	  fp(" (ATmega2561)");
      break;
    }
  } 
  else {
#if defined (__AVR_ATmega168__) || defined(__AVR_ATmega8__)
      fp(" (Fuses not readable on non-P AVR)");
#else
      fp("????");
#endif
  }
}

void print_boot_analysis()
{
  uint8_t verh, verl;
  unsigned int cksum = 0;

  fp("\nBootloader at 0x");
  Serial.print((int)bootaddr, HEX);
  if (!(lock_bits & BOOT_READABLE)) {
    fp(" is not readable\n");
    return;
  }
  verh = pgm_read_byte(FLASHEND);
  verl = pgm_read_byte(FLASHEND-1);
  if (verh == 0xFF || verh == 0x00) {
    fp(" has no version\n");
  } else {
    fp(" looks like version ");
    Serial.print(verh, DEC);
    Serial.print('.');
    Serial.println(verl, DEC);
  }
  for (uint8_t i=0; i < 16; i+=2) {
    space();
    Serial.print(pgm_read_word(&bootaddr[i]), HEX);
  }
}

void loop()
{
  delay(2000);
  fp("\nCompiled for " __CPUNAME "\n");
  print_serno();  
  cli();
  fuse_bits_low = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
  fuse_bits_extended = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
  fuse_bits_high = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
  lock_bits = boot_lock_fuse_bits_get(GET_LOCK_BITS);
  sig1 = boot_signature_byte_get(0);
  sig2 = boot_signature_byte_get(2);
  sig3 = boot_signature_byte_get(4);
  oscal = boot_signature_byte_get(1);
  sei();

  fp("\nFuse bits (L/H/E): ");
  Serial.print(fuse_bits_low, HEX);
  space();
  Serial.print(fuse_bits_high, HEX);
#if !defined(__AVR_ATmega8__)
  space();
  Serial.print(fuse_bits_extended, HEX);
#endif
  fp("\nLock bits:         ");
  Serial.print(lock_bits, HEX);
  print_signature();
  fp("\nOscal:             ");
  Serial.println(oscal, HEX);
  Serial.println();

  print_fuse_low();
  print_fuse_high();
  print_fuse_extended();
  print_lock_bits();
  print_boot_analysis();

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
  defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__)
#elif defined(__AVR_ATmega8__)
#endif
    while (1) {
      if (Serial.available()) {
        int c;
        while ((c = Serial.read()) > 0) {
          Serial.print(c, DEC);
          space();
          delay(1);
        }        
        break;
      }
    }
}



