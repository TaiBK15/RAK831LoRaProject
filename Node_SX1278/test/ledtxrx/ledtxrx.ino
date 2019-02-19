#define LEDTX_U 3
#define LEDRX_U A3

#define LEDTX_S 5
#define LEDRX_S 4
void setup() {
  // put your setup code here, to run once:
  pinMode(LEDTX_U, OUTPUT);
  pinMode(LEDRX_U, OUTPUT);
  pinMode(LEDTX_S, OUTPUT);
  pinMode(LEDRX_S, OUTPUT);
}

void loop() {
  for (int i = 0; i <5; i++) {
    digitalWrite(LEDTX_U, HIGH);  
    delay(100);       
    digitalWrite(LEDRX_U, HIGH);
    delay(100);
    digitalWrite(LEDTX_S, HIGH);
    delay(100);         
    digitalWrite(LEDRX_S, HIGH);
    delay(100);
    delay(1000);                       
    digitalWrite(LEDTX_U, LOW);  
    delay(100);  
    digitalWrite(LEDRX_U, LOW);
    delay(100);
    digitalWrite(LEDTX_S, LOW);       
    delay(100);  
    digitalWrite(LEDRX_S, LOW);
    delay(1000);  
  }
  
}
