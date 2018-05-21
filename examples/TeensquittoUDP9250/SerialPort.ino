///////////////////////////////////////////////////////////////////////////////////////
//
// Serial Port Processing
///////////////////////////////////////////////////////////////////////////////////////

//==================================================

void getPacket() {
  payload[0] = '\0';
  getQ(&quant[0],&quant[1],&quant[2],&quant[3]);
  getYawPitchRoll(ypr);
  MST_PrintVals[0] = ypr[0];    // double         utcTime;      ///  composit utc time
  MST_PrintVals[1] = ypr[1];    // volatile uint32_t IMU_RX_time
  MST_PrintVals[2] = ypr[2];    // volatile uint32_t GPS_RX_time
  MST_PrintVals[3] = Imu.getTemperature_C();   // Eigen::Matrix<double,3,1> RefLLA = Eigen::Matrix<double,3,1>::Zero();

  //convert to payload packet
  char stemp[30];
  for(int i=0; i<4; i++){
    dtostrf(MST_PrintVals[i], 10, 6, stemp);
    strcat(payload, stemp);
    strcat(payload, ",");
  }
  strcat(payload, "\n");
  //Serial.print(payload);
}
