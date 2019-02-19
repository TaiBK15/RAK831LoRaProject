/*
*   dimmer_light.ino
*   Kwang Ng
*/

#define GPIO0   A0
#define GPIO1   A1

void setup() {
    pinMode(GPIO0, OUTPUT);
    pinMode(GPIO1, OUTPUT);
    digitalWrite(GPIO0, LOW);
    digitalWrite(GPIO1, LOW);
}

void loop() {
    
    // change light level
    lightDimmer(100);

    delay(10000);
}

/*
*   Input: percent: Light level 0%, 50%, 100%
*/
void lightDimmer( uint8_t percent ) {
    switch (percent) {
        case 100:
            digitalWrite(GPIO0, HIGH);
            digitalWrite(GPIO1, HIGH); 
            break;
        case 50:
            digitalWrite(GPIO0, HIGH);
            digitalWrite(GPIO1, LOW);
            break;
        case 0:
            digitalWrite(GPIO0, LOW);
            digitalWrite(GPIO1, LOW);
            break;
        default:
            break; 
    }
    delay(1000);
}