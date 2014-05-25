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
// tlačítko pro zobrazení napětí
#define BATTERY_BUTTON 12
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

// počet intervalu, kdy svítí indikace chyby
#define ERROR_ON 10
// počet intervalů, kdy se resetuje čítač pro chybu
#define ERROR_TOP 20
// LED na které se zobrazí chyba potkávacího světla
#define FRONT_LOW_LED LED_0
// LED na které se zobrazí chyba dálkového světla
#define FRONT_HIGH_LED LED_1
// LED na které se zobrazí chyba zadního světla
#define REAR_LED LED_2
// LED na které se zobrazí slabá baterka
#define BATTERY_LED LED_3

// počet intervalů po kolika se bere tlačítko jako sepnuté
#define DEBOUNCE_TOP 3

// počet intervalů, po který se zobrazuje napětí baterky
#define VOLTAGE_SHOW_TOP 250

// počet intervalů, za jak dlouho se měří napětí baterky
#define VOLTAGE_MEASURE_TOP 150

// povolené maximum na měřících rezistorech driverů
// 0,29V / 2,56V * 1024
#define CURRENT_TOP 116
// povolené minimum na měřících rezistorech driverů
// 0,21V / 2,56V * 1024
#define CURRENT_BUTTOM 84

// jednotlivé úrovně napětí baterky
// TODO dopsat správné hodnoty
#define BATTERY_LEVEL_0 100
#define BATTERY_LEVEL_1 120
#define BATTERY_LEVEL_2 140
#define BATTERY_LEVEL_3 160
#define BATTERY_LEVEL_4 180



// čítač blinkrů
byte blink_count = 0;

// čítač pro strobo
byte strobo_count = 0;

// čítač pro debouncing tlačítka pro přepínání předního světla
byte debounce_front_count = 0;

// čítač pro debouncing tlačítka na zobrazení napětí
byte debounce_voltage_count = 0;

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
// true, pokud má baterka slabé napětí
boolean error_battery = false;

// čítač pro blikání chyby
byte error_count = 0;

// napětí baterky
int voltage = 0;

// čítač pro měření baterky
byte voltage_measure_count = 0;

// čítač pro zobrazení napětí baterky
byte voltage_show_count = VOLTAGE_SHOW_TOP; 

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
	pinMode(BATTERY_BUTTON, INPUT_PULLUP);
	pinMode(LED_0, OUTPUT);
	pinMode(LED_1, OUTPUT);
	pinMode(LED_2, OUTPUT);
	pinMode(LED_3, OUTPUT);
	pinMode(LED_4, OUTPUT);
	
#ifndef DEBUG
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	WDT_init();
#else
	Serial.begin(19200);
	Serial.println("Init done.");
#endif
	
	analogReference(INTERNAL);
	voltage_measure_count = VOLTAGE_MEASURE_TOP;
	measureVoltage();
}

void loop() {
	blink();
	measureVoltage();
	debounceVoltage();
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
	} // pro dálkové se chyba maže jinde, aby to dobře fungovalo se strobo režimem
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
		if ((error_front_low == false) && (error_front_high == false) && (error_rear == false) && (error_battery == false)) {
			error = false;
		}
	}
	if (error == true) {
		showError();
	} else {
		if (voltage_show_count > 0) {
			showVoltage();
			voltage_show_count--;
		} else {
			noLEDs();
		}
	}
	if (frontOn() == true) {
		debounceFront();
		if (front_low_enable == true) {
			digitalWrite(FRONT_LOW_ENABLE, HIGH);
			digitalWrite(FRONT_HIGH_ENABLE, LOW);
			front_low_on = true;
			front_high_on = false;
			error_front_high = false;
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
		error_front_high = false;
	}
	if (breakOn() == true) {
		digitalWrite(REAR_INTENSITY, HIGH);
	} else {
		digitalWrite(REAR_INTENSITY, LOW);
	}
	if ((rearOn() == true) || (breakOn() == true)) {
		digitalWrite(REAR_ENABLE, HIGH);
		rear_on = true;
	} else {
		digitalWrite(REAR_ENABLE, LOW);
		rear_on = false;
	}
#ifdef DEBUG
	Serial.flush();
	delay(50);
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

// debouncing tlačítka pro přepínání potkávací/dálkové
void debounceFront() {
	if (digitalRead(FRONT_BUTTON) == LOW) {
		debounce_front_count++;
		if (debounce_front_count == DEBOUNCE_TOP) {
			front_low_enable = !front_low_enable;
#ifdef DEBUG
			Serial.println("Front low/high switch.");
#endif
		} else if (debounce_front_count > 100) {
			debounce_front_count = 100;
		}
	} else {
		debounce_front_count = 0;
	}
}

// debouncing tlačítka pro zobrazení napětí baterky
void debounceVoltage() {
	if (digitalRead(BATTERY_BUTTON) == LOW) {
		debounce_voltage_count++;
		if (debounce_voltage_count == DEBOUNCE_TOP) {
			voltage_show_count = VOLTAGE_SHOW_TOP;
		} else if (debounce_voltage_count > 100) {
			debounce_voltage_count = 100;
		}
	} else {
		debounce_voltage_count = 0;
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
	if (error_battery == true) {
		Serial.println("Low battery.");
		Serial.flush();
		delay(100);
	}
#endif
	if (error_count <= ERROR_ON) {
		if (error_front_low == true) {
			digitalWrite(FRONT_LOW_LED, HIGH);
		}
		if (error_front_high == true) {
			digitalWrite(FRONT_HIGH_LED, HIGH);
		}
		if (error_rear == true) {
			digitalWrite(REAR_LED, HIGH);
		}
		if (error_battery == true) {
			digitalWrite(BATTERY_LED, HIGH);
		}
	} else {
		noLEDs();
		if (error_count >= ERROR_TOP) {
			error_count = 0;
		}
	}
	error_count++;
}

// vypnutí informačních LED
void noLEDs() {
	digitalWrite(LED_0, LOW);
	digitalWrite(LED_1, LOW);
	digitalWrite(LED_2, LOW);
	digitalWrite(LED_3, LOW);
	digitalWrite(LED_4, LOW);
}

void showVoltage() {
	// použít pole a for cyklus by neuškodilo, ale má to pro takhle malé pole smysl?...
#ifdef DEBUG
	Serial.print("Voltage: ");
	Serial.println(voltage);
	Serial.print("Battery level: ");
#endif
	if (voltage >= BATTERY_LEVEL_0) {
		digitalWrite(LED_0, HIGH);
#ifdef DEBUG
	Serial.print("I");
#endif
	} else {
		digitalWrite(LED_0, LOW);
	}
	if (voltage >= BATTERY_LEVEL_1) {
		digitalWrite(LED_1, HIGH);
#ifdef DEBUG
	Serial.print("I");
#endif
	} else {
		digitalWrite(LED_1, LOW);
	}
	if (voltage >= BATTERY_LEVEL_2) {
		digitalWrite(LED_2, HIGH);
#ifdef DEBUG
	Serial.print("I");
#endif
	} else {
		digitalWrite(LED_2, LOW);
	}
	if (voltage >= BATTERY_LEVEL_3) {
		digitalWrite(LED_3, HIGH);
#ifdef DEBUG
	Serial.print("I");
#endif
	} else {
		digitalWrite(LED_3, LOW);
	}
	if (voltage >= BATTERY_LEVEL_4) {
		digitalWrite(LED_4, HIGH);
#ifdef DEBUG
	Serial.print("I");
#endif
	} else {
		digitalWrite(LED_4, LOW);
	}
#ifdef DEBUG
	Serial.println();
#endif
}

// měření napětí baterky
void measureVoltage() {
	voltage_measure_count++;
	if (voltage_measure_count >= VOLTAGE_MEASURE_TOP) {
		analogRead(BATTERY_MEASURE);
		voltage = analogRead(BATTERY_MEASURE);
		if (voltage < BATTERY_LEVEL_0) {
			error = true;
			error_battery = true;
		} else {
			error_battery = false;
		}
		voltage_measure_count = 0;
#ifdef DEBUG
		Serial.println("Measuring battery");
		Serial.print("Voltage: ");
		Serial.println(voltage);
		Serial.flush();
#endif
	}
}