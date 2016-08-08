const byte WDIR = A0;

void setup() {
    pinMode(WDIR, INPUT);
}

int32_t get_wind_direction() {
	uint32_t adc;

    Serial.begin(115200);

    analogReference(AR_EXTERNAL)

	adc = analogRead(WDIR); // get the current reading from the sensor

    Serial.print(adc);
    Serial.print(" ");

	// The following table is ADC readings for the wind direction sensor output, sorted from low to high.
	// Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
	// Note that these are not in compass degree order! See Weather Meters datasheet for more information.




	if (adc < 380) return 113;
	if (adc < 393) return 68;
	if (adc < 414) return 90;
	if (adc < 456) return 158;
	if (adc < 508) return 135;
	if (adc < 551) return 203;
	if (adc < 615) return 180;
	if (adc < 680) return 23;
	if (adc < 746) return 45;
	if (adc < 801) return 248;
	if (adc < 833) return 225;
	if (adc < 878) return 338;
	if (adc < 913) return 0;
	if (adc < 940) return 293;
	if (adc < 967) return 315;
	if (adc < 990) return 270;

	return -1;
}

void loop() {
    int32_t direction = get_wind_direction();

    Serial.println(direction);

    delay(250);
}

// vim: set ft=cpp:
