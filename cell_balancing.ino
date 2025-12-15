// 아두이노 우노 예제 코드
// A0~A3 : 분압 전압 측정 (400Ω 아래, 4kΩ 위)
// D8~D11 : 릴레이 모듈 ON/OFF (5초 간격 전체 토글)
const int NUM_CELLS = 4;
const int analogPins[4] = {A0, A1,A2, A3};  // 아날로그 입력 핀
const int relayPins[4]  = {4,6,7,5};    // 릴레이 출력 핀 4675
//4675
// ADC 관련 상수
const float Vref = 5.0;      // 아두이노 기준 전압(일반적으로 5V)
const int   ADC_MAX = 1023;  // 10비트 ADC 최대값

const float OVERVOLTAGE_V = 5.0;
const float UNDERVOLTAGE_V = 3.0;
const float BALANCE_DIFFERENCE = 0.12;

// 분압 저항 값 (옴)
const float R_bottom = 400.0;   // GND 쪽 400Ω
const float R_top    = 4000.0;  // 위쪽 4kΩ

// 릴레이 토글용
unsigned long lastCutoffMillis   = 0;      // 마지막 사이클 시작 시각
unsigned long cutoffStartMillis  = 0;      // 이번 5초 OFF 시작 시각
bool inCutoff = false;                     // 지금 5초 OFF 모드인지 여부

const unsigned long CUTOFF_INTERVAL = 60000; // 5분 (300,000ms)
const unsigned long CUTOFF_DURATION = 5000;   // 5초  (5,000ms)


// 배터리 전압 측정 함수
float readBatteryVoltage(int pin_vbat) {
  int raw = analogRead(pin_vbat);        // 0~1023
  float vA0 = (raw * Vref) / ADC_MAX;      // A0 전압
  float vBat = vA0 * (R_top + R_bottom) / R_bottom;     // 분압 환산
  return vBat;
}

// 릴레이 ON/OFF 함수
const bool RELAY_ACTIVE_LOW = true;
void setRelay(int ch, bool on) {
  int pin = relayPins[ch];

  if (RELAY_ACTIVE_LOW)
    digitalWrite(pin, on ? LOW : HIGH);   // LOW = ON
  else
    digitalWrite(pin, on ? HIGH : LOW);   // HIGH = ON
}

void setup() {
  Serial.begin(9600);

  // 릴레이 핀 출력 설정
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  // 처음엔 OFF
  }


  // 아날로그 핀은 기본으로 입력이므로 별도 설정 불필요
}

void loop() {
  
  /*
  for (int i = 0; i < NUM_CELLS; i++) {
    Serial.print("Relay ");
    Serial.print(i);
    Serial.println(" ON");

    setRelay(i, true);   // 릴레이 i ON
    delay(1000);         // 1초 동안 유지

    Serial.print("Relay ");
    Serial.print(i);
    Serial.println(" OFF");

    setRelay(i, false);  // 릴레이 i OFF
    delay(1000);         // 1초 쉬고 다음 릴레이로
  }
  */
  
  /* 밸런싱 없이 방전
  float vBat[NUM_CELLS];

  // 각 배터리 전압 출력
  Serial.println("-------------------Start--------------");

  for (int i = 0; i < NUM_CELLS; i++){
    vBat[i] = readBatteryVoltage(analogPins[i]);
    Serial.print("Battery ");
    Serial.print(i);
    Serial.print(" Voltage: ");
    Serial.print(vBat[i]);
    Serial.println(" V");
  } 

  // ---- Python용 데이터 패킷 출력 ----
  Serial.print("DATA");
  for (int i = 0; i < NUM_CELLS; i++){
    Serial.print(",");
    Serial.print(vBat[i], 3);
  }
  Serial.println();

  for (int i = 0; i < NUM_CELLS; i++){
    setRelay(i, true);
    Serial.print("Relay "); Serial.print(i); Serial.println(" ON");
  }
  delay(400);
  */
  
  
  //방전 시 밸런싱
  float vBat[NUM_CELLS];

  // 각 배터리 전압 출력
    Serial.println("-------------------Start--------------");

  for (int i = 0; i < NUM_CELLS; i++){
    vBat[i] = readBatteryVoltage(analogPins[i]);
    Serial.print("Battery ");
    Serial.print(i);
    Serial.print(" Voltage: ");
    Serial.print(vBat[i]);
    Serial.println(" V");
  } 

  // ---- Python용 데이터 패킷 출력 ----
  Serial.print("DATA");
  for (int i = 0; i < NUM_CELLS; i++){
    Serial.print(",");
    Serial.print(vBat[i], 3);
  }
  Serial.println();

  float vmax = vBat[0];
  float vmin = vBat[0];
  for (int i=0; i < NUM_CELLS; i++){
    if (vBat[i] > vmax) vmax = vBat[i];
    if (vBat[i] < vmin) vmin = vBat[i];
  }

  
  
  // 최고전압셀, 최저전압셀의 차이가 BALANCE_DIFFERENCE 이상일 때만 자동 밸런싱
  float diff = vmax - vmin;
  Serial.print("Voltage difference : "); Serial.println(diff);
  if (diff > BALANCE_DIFFERENCE){
    for (int i = 0; i < NUM_CELLS; i++){
      if (vBat[i] <= (vmax - BALANCE_DIFFERENCE) && vBat[i] < OVERVOLTAGE_V && vBat[i] > UNDERVOLTAGE_V){
      //if (vBat[i] >= (vmin + BALANCE_DIFFERENCE)){
        setRelay(i, false); Serial.print("Relay "); Serial.print(i); Serial.println(" OFF");
      }
      else {
        setRelay(i, true);
        Serial.print("Relay "); Serial.print(i); Serial.println(" ON");
      }
    }
  }
  else {
    for (int i = 0; i < NUM_CELLS; i++){
        setRelay(i, true);
        Serial.print("Relay "); Serial.print(i); Serial.println(" ON ver2");
    }
  }
  delay(200); // 너무 자주 시리얼 출력되지 않도록 약간 딜레이
  





  /* 충전 시 밸런싱
  unsigned long currentMillis = millis();
  Serial.print(currentMillis);
  // --- 5분마다 5초간 전체 릴레이 OFF 상태로 만드는 상태 머신 ---
  if (!inCutoff && (currentMillis - lastCutoffMillis >= CUTOFF_INTERVAL)) {
    inCutoff = true;
    cutoffStartMillis = currentMillis;
    Serial.println("=== 5분 주기 OFF 시작 (5초 동안 모든 릴레이 OFF) ===");
    for (int i = 0; i < NUM_CELLS; i++) {
        setRelay(i, false);
    }
  }

  if (inCutoff && (currentMillis - cutoffStartMillis >= CUTOFF_DURATION)) {
    inCutoff = false;
    lastCutoffMillis = currentMillis;
    Serial.println("=== 5분 주기 OFF 종료, 다시 정상 동작 ===");
  }


  float vBat[NUM_CELLS];

  // 각 배터리 전압 출력
    Serial.println("-------------------Start--------------");

  for (int i = 0; i < NUM_CELLS; i++){
    vBat[i] = readBatteryVoltage(analogPins[i]);
    Serial.print("Battery ");
    Serial.print(i);
    Serial.print(" Voltage: ");
    Serial.print(vBat[i]);
    Serial.println(" V");
  } 

  // ---- Python용 데이터 패킷 출력 ----
  Serial.print("DATA");
  for (int i = 0; i < NUM_CELLS; i++){
    Serial.print(",");
    Serial.print(vBat[i], 3);
  }
  Serial.println();


  for (int i = 0; i < NUM_CELLS; i++) {
    
    if (vBat[i] >= OVERVOLTAGE_V) {
      setRelay(i, false);   // 높은 전압 → OFF
      Serial.print("Relay "); Serial.print(i); Serial.println(" OFF (over 5voltage)");
    }
    else if (vBat[i] < UNDERVOLTAGE_V) {
      //setRelay(i, false);   // 낮은 전압 → OFF
      //Serial.print("Relay "); Serial.print(i); Serial.println(" OFF (under 3voltage)");
    //}
    //else {
      //setRelay(i, true);    // 정상 범위 → ON
      //Serial.print("Relay "); Serial.print(i); Serial.println(" ON (normal)");
    //}

  }
}
  float vmax = vBat[0];
  float vmin = vBat[0];
  for (int i=0; i < NUM_CELLS; i++){
    if (vBat[i] > vmax) vmax = vBat[i];
    if (vBat[i] < vmin) vmin = vBat[i];
  }

  // 최고전압셀, 최저전압셀의 차이가 BALANCE_DIFFERENCE 이상일 때만 자동 밸런싱
  float diff = vmax - vmin;
  Serial.print("Voltage difference : "); Serial.println(diff);
  if (diff > BALANCE_DIFFERENCE){
    for (int i = 0; i < NUM_CELLS; i++){
      if (vBat[i] >= (vmin + BALANCE_DIFFERENCE) && vBat[i] < OVERVOLTAGE_V && vBat[i] > UNDERVOLTAGE_V){
      //if (vBat[i] >= (vmin + BALANCE_DIFFERENCE)){
        setRelay(i, false); Serial.print("Relay "); Serial.print(i); Serial.println(" OFF");
      }
      else {
        if (!inCutoff) {
            setRelay(i, true);
            Serial.print("Relay "); Serial.print(i); Serial.println(" ON");
        }
        else {
            Serial.print("Relay "); Serial.print(i);
            Serial.println(" (cutoff 중: 강제 OFF 유지)");
        }
      }
    }
  }
  delay(200); // 너무 자주 시리얼 출력되지 않도록 약간 딜레이
  */
}

