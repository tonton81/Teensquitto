///////////////////////////////////////////////////////////////////////////////////////
//
// IMU Initialization
///////////////////////////////////////////////////////////////////////////////////////

void initIMU() {
  // EEPROM buffer and variables to load accel and mag bias
  // and scale factors from CalibrateMPU9250.ino
  uint8_t EepromBuffer[48];
  float a_xb, a_xs, a_yb, a_ys, a_zb, a_zs;
  float hxb, hxs, hyb, hys, hzb, hzs;

  // start communication with IMU
  status = Imu.begin();
  if (status < 0) {
    cout.println("IMU initialization unsuccessful");
    cout.println("Check IMU wiring or try cycling power");
    cout.print("Status: ");
    cout.println(status);
    while (1) {}
  }
  cout.println("IMU Connected!");
  // load and set accel and mag bias and scale
  // factors from CalibrateMPU9250.ino
  for (size_t i = 0; i < sizeof(EepromBuffer); i++) {
    EepromBuffer[i] = EEPROM.read(i);
  }
  memcpy(&a_xb, EepromBuffer + 0, 4);
  memcpy(&a_xs, EepromBuffer + 4, 4);
  memcpy(&a_yb, EepromBuffer + 8, 4);
  memcpy(&a_ys, EepromBuffer + 12, 4);
  memcpy(&a_zb, EepromBuffer + 16, 4);
  memcpy(&a_zs, EepromBuffer + 20, 4);
  memcpy(&hxb, EepromBuffer + 24, 4);
  memcpy(&hxs, EepromBuffer + 28, 4);
  memcpy(&hyb, EepromBuffer + 32, 4);
  memcpy(&hys, EepromBuffer + 36, 4);
  memcpy(&hzb, EepromBuffer + 40, 4);
  memcpy(&hzs, EepromBuffer + 44, 4);

  Serial.println("Accelerometer bias and scale factors:");
  cout.print( a_xb, 6); cout.print(", "); Serial.println(a_xs, 6);
  cout.print( a_yb, 6); cout.print(", "); Serial.println(a_ys, 6);
  cout.print( a_zb, 6); cout.print(", "); Serial.println(a_zs, 6);
  Serial.println(" ");
  Serial.println("Magnetometer bias and scale factors:");
  cout.print( hxb, 6); cout.print(", "); Serial.println(hxs, 6);
  cout.print( hyb, 6); cout.print(", "); Serial.println(hys, 6);
  cout.print( hzb, 6); cout.print(", "); Serial.println(hzs, 6);
  Serial.println(" ");

  Imu.setAccelCalX(a_xb, a_xs);
  Imu.setAccelCalY(a_yb, a_ys);
  Imu.setAccelCalZ(a_zb, a_zs);

  Imu.setMagCalX(hxb, hxs);
  Imu.setMagCalY(hyb, hys);
  Imu.setMagCalZ(hzb, hzs);

  //Filter.setInitializationDuration(120000000);
  // setting a 41 Hz DLPF bandwidth
  Imu.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_41HZ);
  // setting SRD to 9 for a 100 Hz update rate
  Imu.setSrd(IMU_SRD);

  // enabling the data ready interrupt
  Imu.enableDataReadyInterrupt();
  // attaching the interrupt to microcontroller pin 1
  pinMode(IMU_INT_PIN, INPUT);
  attachInterrupt(IMU_INT_PIN, runFilter, RISING);

  Imu.calibrateGyro();
  Imu.setSrd(IMU_SRD);
}

void getIMU()  //step 1 get marg filter data
{
    // read the sensor
    Imu.readSensor();
 
    // update the filter
    gx1 = Imu.getGyroX_rads(); gy1 = Imu.getGyroY_rads(); gz1 = Imu.getGyroZ_rads();
    ax1 = Imu.getAccelX_mss(); ay1 = Imu.getAccelY_mss(); az1 = Imu.getAccelZ_mss();
    mx1 = Imu.getMagX_uT();    my1 = Imu.getMagY_uT();    mz1 = Imu.getMagZ_uT();  

    #if MARG != 6
      now = micros();
      _dt = ((now - lastUpdate) * 0.000001);
      lastUpdate = now;
    #endif

     MARGUpdateFilter(gx1, gy1, gz1, ax1, ay1, az1, mx1, my1, mz1);
}



