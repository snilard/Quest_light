/*
Ovládací elektronika ke světlům do velomobilu Quest.
Vytvořeno 23.5.2014
Matěj Novotný

Určeno pro Arduino Micro
*/

#include <avr/sleep.h>
#include <avr/wdt.h>

//#define DEBUG


// enable pin potkávacího světla
#define FRONT_LOW_ENABLE 0
// měřící pin k potkávacímu světlu
#define FRONT_LOW_MEASURE A0
// enable pin dálkového světla
#define FRONT_HIGH_ENABLE 1
// měřící pin dálkového světla
#define FRONT_HIGH_MEASURE A1
// enable pin zadního světla
#define REAR_ENABLE 2
// měřící pin zadního světla
#define REAR_MEASURE A2
// pin připojující mosfet pro změnu intenzity zadního světla
#define REAR_INTENSITY 3
// blikry
#define BLINK_PIN 4
// spínač předního světla 
#define FRONT_SWITCH 5
// spínaš strobo režimu
#define STROBO_SWITCH 6
// tlačítko pro přepnutí potkávací/dálkové
#define FRONT_BUTTON 7
// spínač zadního světla
#define REAR_SWITCH 8
// spínač brzdového/mlhového světla
#define BREAK_SWITCH 9
// měřící pin na napětí baterky
#define BATTERY_MEASURE A3
// informační LED
#define LED_0 10
#define LED_1 11
#define LED_2 12
#define LED_3 13
#define LED_4 A4

// počet intervalů, kdy svítí blinkry
#define BLINK_ON 10
// počet intervalů, kdy se resetuje čítač pro blinkry
#define BLINK_TOP 25

// počet intervalů, kdy svítí strobo
#define STROBO_ON 3
// počet intervalů, kdy se resetuje čítač pro strobo
#define STROBO_TOP 12
// počet intervalů po kolika se bere tlačítko jako sepnuté
#define DEBOUNCE_TOP 3

// povolené maximum na měřících rezistorech driverů
// 0,29V / 2,56V * 1024
#define CURRENT_TOP 116
// povolené minimum na měřících rezistorech driverů
// 0,21V / 2,56V * 1024
#define CURRENT_BUTTOM 84



// čítač blinkrů
byte blink_count = 0;

// čítač pro strobo
byte strobo_count = 0;

// čítač pro debouncing
byte debounce_count = 0;

// true, pokud má svítit potkávačka, false pokud dálkové
boolean front_low_enable = true;

// nastaveno na true, pokud má příslušné světlo svítit
boolean front_low_on = false;
boolean front_high_on = false;
boolean rear_on = false;

// true, pokud je v nějakém světle chyba
boolean error = false;
// true, pokud je v příslušném světle chyba
boolean error_front_low = false;
boolean error_front_high = false;
boolean error_rear = false;

void setup() {
	pinMode(FRONT_LOW_ENABLE, OUTPUT);
	pinMode(FRONT_HIGH_ENABLE, OUTPUT);
	pinMode(REAR_ENABLE, OUTPUT);
	pinMode(REAR_INTENSITY, OUTPUT);
	pinMode(BLINK_PIN, OUTPUT);
	pinMode(FRONT_SWITCH, INPUT_PULLUP);
	pinMode(STROBO_SWITCH, INPUT_PULLUP);
	pinMode(FRONT_BUTTON, INPUT_PULLUP);
	pinMode(REAR_SWITCH, INPUT_PULLUP);
	pinMode(BREAK_SWITCH, INPUT_PULLUP);
	pinMode(LED_0, OUTPUT);
	pinMode(LED_1, OUTPUT);
	pinMode(LED_2, OUTPUT);
	pinMode(LED_3, OUTPUT);
	pinMode(LED_4, OUTPUT);
	
#ifndef DEBUG
	set_sleep_mode(SLEEP_MODE_STANDBY);
	WDT_init();
#endif
	analogReference(INTERNAL);
#ifdef DEBUG
	Serial.begin(19200);
	Serial.println("Init done.");
#endif
}

void loop() {
	blink();
	// celé ošetření chyb by se dalo napsat pomocí jedné 8-mi bitové proměnné a bitových operací
	// a bylo by to výrazně efektivnější, ale takhle pomocí těch booleanů mi to přijde takové
	// mnohem pochopitelnější
	
	// pokud je světlo puštěné a změří se špatná hodnota, označí se chyba
	// pokud se naměří správná hodnota nebo je úplně vypnuté, smaže se chyba
	if (front_low_on == true) {
		if (lightFault(FRONT_LOW_MEASURE) == true) {
			error = true;
			error_front_low = true;
		} else {
			error_front_low = false;
		}
	} else {
		error_front_low = false;
	}
	if (front_high_on == true) {
		if (lightFault(FRONT_HIGH_MEASURE) == true) {
			error = true;
			error_front_high = true;
		} else {
			error_front_high = false;
		}
	} else {
		error_front_high = false;
	}
	if (rear_on == true) {
		if (lightFault(REAR_MEASURE) == true) {
			error = true;
			error_rear = true;
		} else {
			error_rear = false;
		}
	} else {
		error_rear = false;
	}
	if (error == true) {
		// pokud už žádné ze světel namá chybu, není už potřeba chybu zobrazovat
		if ((error_front_low == false) && (error_front_high == false) && (error_rear == false)) {
			error = false;
		} else {
			showError();
		}
	}
	if (frontOn() == true) {
		debounceFront();
		if (front_low_enable == true) {
			digitalWrite(FRONT_LOW_ENABLE, HIGH);
			digitalWrite(FRONT_HIGH_ENABLE, LOW);
			front_low_on = true;
			front_high_on = false;
		} else {
			digitalWrite(FRONT_LOW_ENABLE, LOW);
			digitalWrite(FRONT_HIGH_ENABLE, HIGH);
			front_low_on = false;
			front_high_on = true;
		}
	} else if (stroboOn() == true) {
		strobo();
		// přední světla se automaticky vždy zapnou do potkávaček
		front_low_enable = true;
	} else {
		digitalWrite(FRONT_LOW_ENABLE, LOW);
		digitalWrite(FRONT_HIGH_ENABLE, LOW);
		front_low_on = false;
		front_high_on = false;
		// přední světla se automaticky vždy zapnou do potkávaček
		front_low_enable = true;
	}
	if (breakOn() == true) {
		digitalWrite(REAR_INTENSITY, HIGH);
	} else {
		digitalWrite(REAR_INTENSITY, LOW);
	}
	if (rearOn() == true) {
		digitalWrite(REAR_ENABLE, HIGH);
		rear_on = true;
	} else {
		digitalWrite(REAR_ENABLE, LOW);
		rear_on = false;
	}
#ifdef DEBUG
	Serial.flush();
	delay(100);
#else
	enterSleep();
#endif
}

// Vrací true, pokud je sepnutý spínač předního světla
boolean frontOn() {
	return digitalRead(FRONT_SWITCH) == LOW;
}

// Vrací true, pokud je sepnutý spínač pro strobo
boolean stroboOn() {
	return digitalRead(STROBO_SWITCH) == LOW;
}

// Vrací true, pokud je sepnutý spínač zadního světla
boolean rearOn() {
	return digitalRead(REAR_SWITCH) == LOW;
}

// Vrací true, pokud je sepnutý spínač brzdy
boolean breakOn() {
	return digitalRead(BREAK_SWITCH) == LOW;
}


// blikání blinkrů
void blink() {
	if (blink_count <= BLINK_ON) {
		digitalWrite(BLINK_PIN, HIGH);
	} else {
		digitalWrite(BLINK_PIN, LOW);
		if (blink_count >= BLINK_TOP) {
			blink_count = 0;
		}
	}
	blink_count++;
}

// blikání strobo
void strobo() {
	digitalWrite(FRONT_LOW_ENABLE, LOW);
	front_low_on = false;
	if (strobo_count <= STROBO_ON) {
		digitalWrite(FRONT_HIGH_ENABLE, HIGH);
		front_high_on = true;
	} else {
		digitalWrite(FRONT_HIGH_ENABLE, LOW);
		front_high_on = false;
		if (strobo_count >= STROBO_TOP) {
			strobo_count = 0;
		}
	}
	strobo_count++;
}

// debouncing tlačítka a přepínání potkávací/dálkové
void debounceFront() {
	if (digitalRead(FRONT_BUTTON) == LOW) {
		debounce_count++;
		if (debounce_count == DEBOUNCE_TOP) {
			front_low_enable = !front_low_enable;
#ifdef DEBUG
			Serial.println("Front low/high switch.");
#endif
		} else if (debounce_count > 100) {
			debounce_count = 100;
		}
	} else {
		debounce_count = 0;
	}
}


// spánek
void enterSleep(void) {
	cli();
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
}


// přerušení watchdogu
ISR(WDT_vect) {
	WDTCSR |= (1<<WDIE);
}

// inicializace watchdogu
void WDT_init(void) {
	//disable interrupts
	cli();
	//reset watchdog
	wdt_reset();
	//set up WDT interrupt
	//start watchdog timer with 16ms delay
	WDTCSR = (1<<WDCE)|(1<<WDIE)|(1<<WDE);
	//enable global interrupts
	sei();
}

// vrací true pokud světlo na příslušením pinu nefunguje, jak má.
boolean lightFault(int pin) {
	analogRead(pin);
	int current = analogRead(pin);
	if (current > CURRENT_TOP) {
#ifdef DEBUG
		Serial.print("Current on pin ");
		Serial.print(pin);
		Serial.print(": ");
		Serial.println(current);
		Serial.flush();
		delay(100);
#endif
		return true;
	}
	if (current < CURRENT_BUTTOM) {
#ifdef DEBUG
		Serial.print("Current on pin ");
		Serial.print(pin);
		Serial.print(": ");
		Serial.println(current);
		Serial.flush();
		delay(100);
#endif
		return true;
	}
	return false;
}

// ukazuje nastalou chybu
void showError() {
#ifdef DEBUG
	if (error_front_low == true) {
		Serial.println("Front low error.");
		Serial.flush();
		delay(100);
	}
	if (error_front_high == true) {
		Serial.println("Front high error.");
		Serial.flush();
		delay(100);
	}
	if (error_rear == true) {
		Serial.println("Rear error.");
		Serial.flush();
		delay(100);
	}
#endif
}