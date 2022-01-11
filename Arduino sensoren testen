float sensor1, sensor2, sensor3, sensor4;

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
}

void loop() {
  sensor1 = touchRead(T0); //pin 2
  sensor2 = touchRead(T2); //pin 4
  sensor3 = touchRead(T4); //pin 13
  sensor4 = touchRead(T7); //pin 27
  Serial.println(sensor1);
  Serial.println(sensor2);
  Serial.println(sensor3);
  Serial.println(sensor4);
  //values.add(23);
  Serial.println("-------------");
  delay(2000);
}
