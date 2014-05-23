/*
Ovládací elektronika ke světlům do velomobilu Quest.
Vytvořeno 23.5.2014
Matěj Novotný

Určeno pro Arduino Micro
*/

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
#define BACK_SWITCH 8
// spínač brzdového/mlhového světla
#define BREAK_SWITCH 9
// měřící pin na napětí baterky
#define BATTERY_MEASURE A3

// počet intervalů, kdy svítí blinkry
#define BLINK_ON 10
// počet intervalů, kdy se resetuje čítač pro blinkry
#define BLINK_TOP 25

// počet intervalů, kdy svítí strobo
#define STROBO_ON 3
// počet intervalů, kdy se resetuje čítač pro strobo
#define STROBO_TOP 12


// čítač blinkrů
byte blink_count = 0;

// čítač pro strobo
byte strobo_count = 0;

// true, pokud má svítit potkávačka, false pokud dálkové
boolean front_low_enable = true;

void setup() {
	pinMode(FRONT_LOW_ENABLE, OUTPUT);
	pinMode(FRONT_HIGH_ENABLE, OUTPUT);
	pinMode(REAR_ENABLE, OUTPUT);
	pinMode(REAR_INTENSITY, OUTPUT);
	pinMode(BLINK_PIN, OUTPUT);
	pinMode(FRONT_SWITCH, INPUT_PULLUP);
	pinMode(STROBO_SWITCH, INPUT_PULLUP);
	pinMode(FRONT_BUTTON, INPUT_PULLUP);
	pinMode(BACK_SWITCH, INPUT_PULLUP);
	pinMode(BREAK_SWITCH, INPUT_PULLUP);
}

void loop() {
	blink();
	if (frontOn() == true) {
		if (front_low_enable == true) {
			digitalWrite(FRONT_LOW_ENABLE, HIGH);
			digitalWrite(FRONT_HIGH_ENABLE, LOW);
		} else {
			digitalWrite(FRONT_LOW_ENABLE, LOW);
			digitalWrite(FRONT_HIGH_ENABLE, HIGH);
		}
	} else if (stroboOn() == true) {
		strobo();
	} else {
		digitalWrite(FRONT_LOW_ENABLE, LOW);
		digitalWrite(FRONT_HIGH_ENABLE, LOW);
	}
	if (breakOn() == true) {
		digitalWrite(REAR_INTENSITY, HIGH);
	} else {
		digitalWrite(REAR_INTENSITY, LOW);
	}
	if (rearOn() == true) {
		digitalWrite(REAR_ENABLE, HIGH);
	} else {
		digitalWrite(REAR_ENABLE, LOW);
	}
	delay(16);
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
	return digitalRead(BACK_SWITCH) == LOW;
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
	if (strobo_count <= STROBO_ON) {
		digitalWrite(FRONT_HIGH_ENABLE, HIGH);
	} else {
		digitalWrite(FRONT_HIGH_ENABLE, LOW);
		if (strobo_count >= STROBO_TOP) {
			strobo_count = 0;
		}
	}
	strobo_count++;
}