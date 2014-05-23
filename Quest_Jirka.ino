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
}